#pragma once
#include "ITelemetrySource.h"
#include <fstream>
#include <string>

class FileSource : public ITelemetrySource {
public:
    FileSource(std::string sensor_id, std::string path, bool loop = true);
    std::optional<TelemetryFrame> read() override;
    std::string_view name() const override { return sensor_id_; }

private:
    std::string sensor_id_;
    std::string path_;
    bool loop_;
    std::ifstream file_;
    void reopen();
};
