#include "StateManager.h"
#include "DaemonBus.h"
#include "DataProcessor.h"
#include "SimulatedSource.h"
#include "FileSource.h"
#include "TelemetryReceiver.h"
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#ifdef STUB_DBUS
static inline int sd_notify(int, const char*) noexcept { return 0; }
#else
#include <systemd/sd-daemon.h>
#endif

static std::atomic<bool> g_quit{false};
static void sigHandler(int) { g_quit = true; }

int main() {
    std::signal(SIGTERM, sigHandler);
    std::signal(SIGINT,  sigHandler);

    auto sm  = std::make_shared<StateManager>();
    auto bus = std::make_unique<DaemonBus>(sm);

    sm->setStateChangedCallback([&](DaemonState next, DaemonState prev) {
        bus->emitStateChanged(next, prev);
    });
    sm->setAlarmCallback([&](const std::string& sid, double val, double thr) {
        bus->emitAlarmRaised(sid, val, thr);
    });

    if (!bus->start()) {
        std::cerr << "edge-ipc-daemon: failed to register D-Bus service\n";
        return 1;
    }

    const char* path = std::getenv("TELEMETRY_PATH");
    std::unique_ptr<ITelemetrySource> source;
    if (path)
        source = std::make_unique<FileSource>("sensor0", path);
    else
        source = std::make_unique<SimulatedSource>("sensor0", 50.0, 10.0, 0.05, 4.0);

    TelemetryReceiver rx;
    DataProcessor     proc(10, 80.0, 20.0, 3);

    std::atomic<bool> producer_stop{false};
    std::thread producer([&]() {
        while (!producer_stop.load()) {
            auto frame = source->read();
            if (frame) rx.push(*frame);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    sd_notify(0, "READY=1");
    std::cout << "edge-ipc-daemon: ready\n";

    auto last_watchdog = std::chrono::steady_clock::now();

    while (!g_quit) {
        TelemetryFrame frame;
        while (rx.pop(frame)) {
            auto metric = proc.process(frame);
            if (metric.alarm_flag)
                sm->fault(metric.sensor_id, metric.filtered_value, 80.0);
        }

        auto now = std::chrono::steady_clock::now();
        if (now - last_watchdog >= std::chrono::seconds(5)) {
            sd_notify(0, "WATCHDOG=1");
            last_watchdog = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    sd_notify(0, "STOPPING=1");
    producer_stop.store(true);
    producer.join();
    sm->shutdown();
    sm->setStateChangedCallback(nullptr);
    sm->setAlarmCallback(nullptr);
    bus->stop();
    return 0;
}
