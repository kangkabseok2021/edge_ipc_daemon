#pragma once
#include "DaemonState.h"
#include <memory>
#include <string>

class StateManager;

class DaemonBus {
public:
    explicit DaemonBus(std::weak_ptr<StateManager> sm);
    ~DaemonBus();

    bool start();
    void stop();

    void emitStateChanged(DaemonState new_state, DaemonState prev_state);
    void emitAlarmRaised(const std::string& sensor_id, double value, double threshold);

    struct Impl;

private:
    std::unique_ptr<Impl> impl_;
};
