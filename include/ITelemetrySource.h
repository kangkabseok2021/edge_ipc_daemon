#pragma once
#include "TelemetryFrame.h"
#include <optional>
#include <string_view>

class ITelemetrySource {
public:
    virtual std::optional<TelemetryFrame> read() = 0;
    virtual std::string_view name() const = 0;
    virtual ~ITelemetrySource() = default;
};
