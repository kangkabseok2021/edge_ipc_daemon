#include "DataProcessor.h"
DataProcessor::DataProcessor(size_t w, double h, double l, int k)
    : filter_(w), detector_(h, l, k) {}
ProcessedMetric DataProcessor::process(const TelemetryFrame& f) { return {f.sensor_id, 0.0, false}; }
void DataProcessor::reset() {}
