#include "SimulatedSource.h"
#include <chrono>

SimulatedSource::SimulatedSource(std::string sid, double mu, double sigma,
                                 double spike_prob, double spike_magnitude)
    : sensor_id_(std::move(sid))
    , mu_(mu), sigma_(sigma)
    , spike_prob_(spike_prob), spike_magnitude_(spike_magnitude)
    , rng_(std::random_device{}())
    , dist_(mu, sigma)
    , uniform_(0.0, 1.0)
{}

std::optional<TelemetryFrame> SimulatedSource::read() {
    double val = dist_(rng_);
    if (uniform_(rng_) < spike_prob_)
        val = mu_ + spike_magnitude_ * sigma_;
    return TelemetryFrame{sensor_id_, val, std::chrono::steady_clock::now()};
}
