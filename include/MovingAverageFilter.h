#pragma once
#include "IDataFilter.h"
#include <vector>
#include <cstddef>

class MovingAverageFilter : public IDataFilter {
public:
    explicit MovingAverageFilter(size_t window);
    double apply(double sample) override;
    void reset() override;

private:
    size_t window_;
    std::vector<double> buf_;
    size_t head_{0};
    size_t count_{0};
    double sum_{0.0};
};
