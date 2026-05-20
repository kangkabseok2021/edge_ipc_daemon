#pragma once
#include "IDataFilter.h"

class ThresholdDetector : public IDataFilter {
public:
    ThresholdDetector(double high_watermark, double low_watermark, int k = 3);
    double apply(double sample) override;  // pass-through; updates alarm state
    void reset() override;
    bool alarm() const { return alarm_; }

private:
    double high_;
    double low_;
    int k_;
    int consecutive_{0};
    bool alarm_{false};
};
