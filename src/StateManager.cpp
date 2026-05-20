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
    DaemonState cur = state_.load(std::memory_order_acquire);
    if (cur == DaemonState::RUNNING)
        throw std::logic_error("start() called from RUNNING state");
    if (cur == DaemonState::SHUTDOWN)
        throw std::logic_error("start() called from SHUTDOWN state");
    transition(DaemonState::RUNNING);
}

void StateManager::halt() {
    if (state_.load(std::memory_order_acquire) == DaemonState::SHUTDOWN) return;
    transition(DaemonState::IDLE);
}

void StateManager::fault(const std::string& sensor_id, double value, double threshold) {
    if (state_.load(std::memory_order_acquire) != DaemonState::RUNNING) return;
    transition(DaemonState::FAULT);
    AlarmCb cb;
    { std::lock_guard<std::mutex> lk(cb_mu_); cb = alarm_cb_; }
    if (cb) cb(sensor_id, value, threshold);
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
