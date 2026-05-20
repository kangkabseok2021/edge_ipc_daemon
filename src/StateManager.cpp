#include "StateManager.h"
#include <stdexcept>

void StateManager::setStateChangedCallback(StateChangedCb cb) {
    std::lock_guard<std::mutex> lk(cb_mu_);
    state_changed_cb_ = std::move(cb);
}

void StateManager::setAlarmCallback(AlarmCb cb) {
    std::lock_guard<std::mutex> lk(cb_mu_);
    alarm_cb_ = std::move(cb);
}

void StateManager::start() {
    DaemonState prev = state_.load(std::memory_order_acquire);
    while (true) {
        if (prev == DaemonState::RUNNING)
            throw std::logic_error("start() called from RUNNING state");
        if (prev == DaemonState::SHUTDOWN)
            throw std::logic_error("start() called from SHUTDOWN state");
        if (state_.compare_exchange_weak(prev, DaemonState::RUNNING,
                                         std::memory_order_acq_rel,
                                         std::memory_order_acquire))
            break;
        // prev updated to current value by compare_exchange_weak — retry
    }
    StateChangedCb cb;
    { std::lock_guard<std::mutex> lk(cb_mu_); cb = state_changed_cb_; }
    if (cb) cb(DaemonState::RUNNING, prev);
}

void StateManager::halt() {
    if (state_.load(std::memory_order_acquire) == DaemonState::SHUTDOWN) return;
    transition(DaemonState::IDLE);
}

void StateManager::fault(const std::string& sensor_id, double value, double threshold) {
    DaemonState expected = DaemonState::RUNNING;
    if (!state_.compare_exchange_strong(expected, DaemonState::FAULT,
                                        std::memory_order_acq_rel,
                                        std::memory_order_acquire)) {
        return;
    }
    StateChangedCb scb;
    { std::lock_guard<std::mutex> lk(cb_mu_); scb = state_changed_cb_; }
    if (scb) scb(DaemonState::FAULT, DaemonState::RUNNING);
    AlarmCb acb;
    { std::lock_guard<std::mutex> lk(cb_mu_); acb = alarm_cb_; }
    if (acb) acb(sensor_id, value, threshold);
}

void StateManager::shutdown() {
    transition(DaemonState::SHUTDOWN);
}

void StateManager::transition(DaemonState next) {
    DaemonState prev = state_.exchange(next, std::memory_order_acq_rel);
    StateChangedCb cb;
    { std::lock_guard<std::mutex> lk(cb_mu_); cb = state_changed_cb_; }
    if (cb) cb(next, prev);
}
