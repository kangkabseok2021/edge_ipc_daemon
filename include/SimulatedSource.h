#pragma once
#include "ITelemetrySource.h"
#include <random>
#include <string>

class SimulatedSource : public ITelemetrySource {
public:
    SimulatedSource(std::string sensor_id,
                    double mu = 50.0, double sigma = 10.0,
                    double spike_prob = 0.0, double spike_magnitude = 5.0);
    std::optional<TelemetryFrame> read() override;
    std::string_view name() const override { return sensor_id_; }

private:
    std::string sensor_id_;
    double mu_, sigma_, spike_prob_, spike_magnitude_;
    std::mt19937 rng_;
    std::normal_distribution<double> dist_;
    std::uniform_real_distribution<double> uniform_;
};
