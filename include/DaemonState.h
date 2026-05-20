#pragma once
#include <string_view>

enum class DaemonState { IDLE, RUNNING, FAULT, SHUTDOWN };

inline std::string_view stateToString(DaemonState s) {
    switch (s) {
        case DaemonState::IDLE:     return "IDLE";
        case DaemonState::RUNNING:  return "RUNNING";
        case DaemonState::FAULT:    return "FAULT";
        case DaemonState::SHUTDOWN: return "SHUTDOWN";
    }
    return "UNKNOWN";
}
