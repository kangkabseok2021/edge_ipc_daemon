#pragma once
#include "DaemonState.h"
#include <atomic>
#include <functional>
#include <mutex>
#include <string>

class StateManager {
public:
    using StateChangedCb = std::function<void(DaemonState next, DaemonState prev)>;
    using AlarmCb        = std::function<void(const std::string& sensor_id,
                                              double value, double threshold)>;

    void setStateChangedCallback(StateChangedCb cb);
    void setAlarmCallback(AlarmCb cb);

    void start();    // IDLE/FAULT → RUNNING  (throws std::logic_error from RUNNING/SHUTDOWN)
    void halt();     // any → IDLE            (no-op from SHUTDOWN)
    void fault(const std::string& sensor_id, double value, double threshold);  // RUNNING → FAULT
    void shutdown(); // any → SHUTDOWN

    DaemonState state() const { return state_.load(std::memory_order_acquire); }

private:
    void transition(DaemonState next);

    std::atomic<DaemonState> state_{DaemonState::IDLE};
    mutable std::mutex cb_mu_;
    StateChangedCb state_changed_cb_;
    AlarmCb        alarm_cb_;
};
