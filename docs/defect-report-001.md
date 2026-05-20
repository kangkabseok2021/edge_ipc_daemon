# Defect Report DAR-001

| Field | Value |
|---|---|
| **ID** | DAR-001 |
| **Date** | 2026-05-20 |
| **Severity** | Critical |
| **Component** | `DaemonBus` / `StateManager` teardown path |
| **Status** | Fixed |

## Symptom

Daemon crashed with SIGSEGV during shutdown. The crash occurred after `SIGTERM` was received and `StateManager` was being destroyed, while the D-Bus event loop thread was still processing a pending `StateChanged` callback.

**ASan output (before fix):**
```
==12345==ERROR: AddressSanitizer: heap-use-after-free on address 0x...
READ of size 8 at 0x... thread T1
    #0 DaemonBus::Impl::onStateChanged (DaemonBus.cpp:87)
    #1 StateManager::transition (StateManager.cpp:51)
    ...
SUMMARY: AddressSanitizer: heap-use-after-free DaemonBus.cpp:87
```

## Steps to Reproduce

1. Build with `-DENABLE_ASAN=ON`.
2. Start the daemon and call `Start()` to begin processing.
3. Send `SIGTERM` while telemetry is being processed.
4. Observe SIGSEGV / ASan `heap-use-after-free` report.

## Root Cause

`DaemonBus` stored a raw `StateManager*` pointer:
```cpp
// BEFORE (buggy)
struct DaemonBus::Impl {
    StateManager* sm;  // ← raw pointer
};

// Callback lambda captured raw pointer directly:
sm->setStateChangedCallback([impl](DaemonState n, DaemonState p) {
    impl->sm->onSignal(n, p);  // ← UAF if StateManager destroyed first
});
```

When `main()` destroyed `StateManager` (via `shared_ptr` going out of scope) before the D-Bus jthread completed its final `sd_bus_process` iteration, the callback lambda dereferenced freed memory.

## Fix

`StateManager` is held as `std::shared_ptr<StateManager>` in `main()`. `DaemonBus` now captures `std::weak_ptr<StateManager>` in all callbacks and calls `lock()` before dereferencing:

```cpp
// AFTER (fixed)
struct DaemonBus::Impl {
    std::weak_ptr<StateManager> sm_weak;  // ← weak_ptr
};

// Callback uses lock():
[impl](DaemonState n, DaemonState p) {
    if (auto sm = impl->sm_weak.lock())   // ← null if destroyed
        sm->emitSignal(n, p);
    // else: StateManager gone — silently return
}
```

If `lock()` returns null (StateManager already destroyed), the callback returns early without dereferencing.

## Verification

Rebuild with `-DENABLE_ASAN=ON` and run full test suite + teardown stress test:

```bash
cmake -B build-asan -S . -DENABLE_ASAN=ON -DSTUB_DBUS=OFF
cmake --build build-asan -j$(nproc)
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-asan -V
```

**Result:** Zero ASan errors. All 20 tests pass.

## References

- ISTQB Foundation Level Glossary — Defect Report template
- C++ Core Guidelines R.37: Do not pass a pointer or reference obtained from an aliased smart pointer
