#include "SimulatedSource.h"
SimulatedSource::SimulatedSource(std::string sid, double mu, double sigma,
                                 double sp, double sm_)
    : sensor_id_(std::move(sid)), mu_(mu), sigma_(sigma), spike_prob_(sp),
      spike_magnitude_(sm_), rng_(std::random_device{}()), dist_(mu, sigma),
      uniform_(0.0, 1.0) {}
std::optional<TelemetryFrame> SimulatedSource::read() { return std::nullopt; }
