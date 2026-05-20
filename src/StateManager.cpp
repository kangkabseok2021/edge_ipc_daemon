#include "StateManager.h"
void StateManager::setStateChangedCallback(StateChangedCb cb) {
    std::lock_guard<std::mutex> lk(cb_mu_); state_changed_cb_ = std::move(cb);
}
void StateManager::setAlarmCallback(AlarmCb cb) {
    std::lock_guard<std::mutex> lk(cb_mu_); alarm_cb_ = std::move(cb);
}
void StateManager::start() {}
void StateManager::halt() {}
void StateManager::fault(const std::string&, double, double) {}
void StateManager::shutdown() {}
void StateManager::transition(DaemonState) {}
