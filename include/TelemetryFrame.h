#pragma once
#include <chrono>
#include <string>

struct TelemetryFrame {
    std::string sensor_id;
    double value;
    std::chrono::steady_clock::time_point timestamp;
};
