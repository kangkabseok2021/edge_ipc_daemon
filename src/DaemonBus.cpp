#include "DaemonBus.h"
struct DaemonBus::Impl {};
DaemonBus::DaemonBus(std::weak_ptr<StateManager>) : impl_(std::make_unique<Impl>()) {}
DaemonBus::~DaemonBus() = default;
bool DaemonBus::start() { return true; }
void DaemonBus::stop() {}
void DaemonBus::emitStateChanged(DaemonState, DaemonState) {}
void DaemonBus::emitAlarmRaised(const std::string&, double, double) {}
