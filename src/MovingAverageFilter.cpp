#include "MovingAverageFilter.h"
#include <algorithm>

MovingAverageFilter::MovingAverageFilter(size_t window)
    : window_(window), buf_(window, 0.0) {}

double MovingAverageFilter::apply(double sample) {
    if (count_ < window_) ++count_;
    else sum_ -= buf_[head_];
    buf_[head_] = sample;
    sum_ += sample;
    head_ = (head_ + 1) % window_;
    return sum_ / count_;
}

void MovingAverageFilter::reset() {
    std::fill(buf_.begin(), buf_.end(), 0.0);
    head_  = 0;
    count_ = 0;
    sum_   = 0.0;
}
