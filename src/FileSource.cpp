#include "FileSource.h"
FileSource::FileSource(std::string sid, std::string path, bool loop)
    : sensor_id_(std::move(sid)), path_(std::move(path)), loop_(loop) {}
void FileSource::reopen() {}
std::optional<TelemetryFrame> FileSource::read() { return std::nullopt; }
