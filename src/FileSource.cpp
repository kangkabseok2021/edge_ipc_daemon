#include "FileSource.h"
#include <chrono>

FileSource::FileSource(std::string sid, std::string path, bool loop)
    : sensor_id_(std::move(sid)), path_(std::move(path)), loop_(loop) {
    reopen();
}

void FileSource::reopen() {
    file_.close();
    file_.open(path_);
}

std::optional<TelemetryFrame> FileSource::read() {
    std::string line;
    auto fetch = [&]() -> bool {
        while (std::getline(file_, line)) {
            if (!line.empty() && line[0] != '#') return true;
        }
        return false;
    };

    if (!fetch()) {
        if (!loop_) return std::nullopt;
        reopen();
        if (!fetch()) return std::nullopt;
    }
    try {
        double val = std::stod(line);
        return TelemetryFrame{sensor_id_, val, std::chrono::steady_clock::now()};
    } catch (...) {
        return std::nullopt;
    }
}
