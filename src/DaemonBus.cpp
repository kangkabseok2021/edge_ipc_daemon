#include "DaemonBus.h"
#include "StateManager.h"

#ifdef STUB_DBUS
// ─── No-op stub (macOS / CI without libsystemd) ───────────────
struct DaemonBus::Impl {};
DaemonBus::DaemonBus(std::weak_ptr<StateManager>) : impl_(std::make_unique<Impl>()) {}
DaemonBus::~DaemonBus() = default;
bool DaemonBus::start() { return true; }
void DaemonBus::stop() {}
void DaemonBus::emitStateChanged(DaemonState, DaemonState) {}
void DaemonBus::emitAlarmRaised(const std::string&, double, double) {}

#else
// ─── Real sd-bus implementation (Linux + libsystemd) ─────────
#include <systemd/sd-bus.h>
#include <atomic>
#include <chrono>
#include <fstream>
#include <thread>

// Forward declarations for vtable handlers
static int method_start      (sd_bus_message*, void*, sd_bus_error*);
static int method_halt       (sd_bus_message*, void*, sd_bus_error*);
static int method_diagnostics(sd_bus_message*, void*, sd_bus_error*);
static int prop_current_state(sd_bus*, const char*, const char*, const char*,
                               sd_bus_message*, void*, sd_bus_error*);

static const sd_bus_vtable daemon_vtable[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD("Start",          nullptr, nullptr,  method_start,       0),
    SD_BUS_METHOD("Halt",           nullptr, nullptr,  method_halt,        0),
    SD_BUS_METHOD("RunDiagnostics", nullptr, "(ttst)", method_diagnostics, 0),
    SD_BUS_SIGNAL("StateChanged", "ss",  0),
    SD_BUS_SIGNAL("AlarmRaised",  "sdd", 0),
    SD_BUS_PROPERTY("CurrentState", "s", prop_current_state, 0, 0),
    SD_BUS_VTABLE_END
};

struct DaemonBus::Impl {
    sd_bus*      bus{nullptr};
    sd_bus_slot* slot{nullptr};
    std::weak_ptr<StateManager> sm_weak;
    std::atomic<bool>     running{false};
    std::jthread          worker;
    std::atomic<uint64_t> processed_count{0};
    std::chrono::steady_clock::time_point start_time;
};

static uint64_t rss_kb() {
    std::ifstream f("/proc/self/status");
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("VmRSS:", 0) == 0) {
            uint64_t kb = 0;
            sscanf(line.c_str(), "VmRSS: %lu kB", &kb);
            return kb;
        }
    }
    return 0;
}

static int method_start(sd_bus_message* m, void* ud, sd_bus_error* err) {
    auto* impl = static_cast<DaemonBus::Impl*>(ud);
    if (auto sm = impl->sm_weak.lock()) {
        try { sm->start(); }
        catch (const std::exception& e) {
            sd_bus_error_set(err, "org.agntx.Error.InvalidState", e.what());
            return -EINVAL;
        }
    }
    return sd_bus_reply_method_return(m, nullptr);
}

static int method_halt(sd_bus_message* m, void* ud, sd_bus_error* /*err*/) {
    auto* impl = static_cast<DaemonBus::Impl*>(ud);
    if (auto sm = impl->sm_weak.lock()) sm->halt();
    return sd_bus_reply_method_return(m, nullptr);
}

static int method_diagnostics(sd_bus_message* m, void* ud, sd_bus_error* /*err*/) {
    auto* impl = static_cast<DaemonBus::Impl*>(ud);
    uint64_t rss    = rss_kb();
    uint64_t cnt    = impl->processed_count.load(std::memory_order_relaxed);
    uint64_t uptime = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - impl->start_time).count());
    std::string state_str = "IDLE";
    if (auto sm = impl->sm_weak.lock())
        state_str = std::string(stateToString(sm->state()));
    return sd_bus_reply_method_return(m, "(ttst)",
        rss, cnt, state_str.c_str(), uptime);
}

static int prop_current_state(sd_bus* /*b*/, const char* /*path*/,
                               const char* /*iface*/, const char* /*prop*/,
                               sd_bus_message* reply, void* ud, sd_bus_error* /*err*/) {
    auto* impl = static_cast<DaemonBus::Impl*>(ud);
    std::string s = "IDLE";
    if (auto sm = impl->sm_weak.lock())
        s = std::string(stateToString(sm->state()));
    return sd_bus_message_append(reply, "s", s.c_str());
}

DaemonBus::DaemonBus(std::weak_ptr<StateManager> sm)
    : impl_(std::make_unique<Impl>()) {
    impl_->sm_weak    = std::move(sm);
    impl_->start_time = std::chrono::steady_clock::now();
}

DaemonBus::~DaemonBus() { stop(); }

bool DaemonBus::start() {
    int r = sd_bus_open_system(&impl_->bus);
    if (r < 0) return false;

    r = sd_bus_add_object_vtable(impl_->bus, &impl_->slot,
        "/org/agntx/EdgeDaemon",
        "org.agntx.EdgeDaemon1",
        daemon_vtable,
        impl_.get());
    if (r < 0) return false;

    r = sd_bus_request_name(impl_->bus, "org.agntx.EdgeDaemon", 0);
    if (r < 0) return false;

    impl_->running = true;
    impl_->worker = std::jthread([this](std::stop_token st) {
        while (!st.stop_requested()) {
            int rc = sd_bus_process(impl_->bus, nullptr);
            if (rc > 0) continue;
            sd_bus_wait(impl_->bus, 10'000 /* µs = 10ms */);
        }
    });
    return true;
}

void DaemonBus::stop() {
    if (!impl_->running.exchange(false)) return;
    impl_->worker.request_stop();
    impl_->worker.join();  // wait for thread to exit before clearing bus/slot
    if (impl_->slot) { sd_bus_slot_unref(impl_->slot); impl_->slot = nullptr; }
    if (impl_->bus)  { sd_bus_unref(impl_->bus);       impl_->bus  = nullptr; }
}

void DaemonBus::emitStateChanged(DaemonState new_state, DaemonState prev_state) {
    if (!impl_->bus) return;
    sd_bus_emit_signal(impl_->bus,
        "/org/agntx/EdgeDaemon", "org.agntx.EdgeDaemon1", "StateChanged", "ss",
        std::string(stateToString(new_state)).c_str(),
        std::string(stateToString(prev_state)).c_str());
}

void DaemonBus::emitAlarmRaised(const std::string& sensor_id,
                                  double value, double threshold) {
    if (!impl_->bus) return;
    sd_bus_emit_signal(impl_->bus,
        "/org/agntx/EdgeDaemon", "org.agntx.EdgeDaemon1", "AlarmRaised", "sdd",
        sensor_id.c_str(), value, threshold);
}
#endif  // STUB_DBUS
