#include "MovingAverageFilter.h"
MovingAverageFilter::MovingAverageFilter(size_t w) : window_(w), buf_(w, 0.0) {}
double MovingAverageFilter::apply(double) { return 0.0; }
void MovingAverageFilter::reset() {}
