#include "ThresholdDetector.h"
ThresholdDetector::ThresholdDetector(double h, double l, int k) : high_(h), low_(l), k_(k) {}
double ThresholdDetector::apply(double s) { return s; }
void ThresholdDetector::reset() {}
