#pragma once
#include "MovingAverageFilter.h"
#include "ThresholdDetector.h"
#include "TelemetryFrame.h"
#include <string>

struct ProcessedMetric {
    std::string sensor_id;
    double filtered_value;
    bool alarm_flag;
};

class DataProcessor {
public:
    DataProcessor(size_t filter_window = 10,
                  double high_watermark = 80.0,
                  double low_watermark  = 20.0,
                  int k = 3);
    ProcessedMetric process(const TelemetryFrame& frame);
    void reset();

private:
    MovingAverageFilter filter_;
    ThresholdDetector   detector_;
};
