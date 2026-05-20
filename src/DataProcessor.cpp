#include "DataProcessor.h"

DataProcessor::DataProcessor(size_t w, double high, double low, int k)
    : filter_(w), detector_(high, low, k) {}

ProcessedMetric DataProcessor::process(const TelemetryFrame& frame) {
    double filtered = filter_.apply(frame.value);
    detector_.apply(frame.value);
    return {frame.sensor_id, filtered, detector_.alarm()};
}

void DataProcessor::reset() {
    filter_.reset();
    detector_.reset();
}
