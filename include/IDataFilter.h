#pragma once

class IDataFilter {
public:
    virtual double apply(double sample) = 0;
    virtual void reset() = 0;
    virtual ~IDataFilter() = default;
};
