#include "ThresholdDetector.h"

ThresholdDetector::ThresholdDetector(double high, double low, int k)
    : high_(high), low_(low), k_(k) {}

double ThresholdDetector::apply(double sample) {
    if (sample > high_ || sample < low_) {
        if (++consecutive_ >= k_) alarm_ = true;
    } else {
        consecutive_ = 0;
        alarm_ = false;
    }
    return sample;
}

void ThresholdDetector::reset() {
    consecutive_ = 0;
    alarm_        = false;
}
