#pragma once
#include <adapter/output/Logger.h>
#include <LilyGo_AMOLED.h>

class SerialLogger : public Logger {
private:

public:
    SerialLogger(LogLevel level = LogLevel::INFO);
    virtual ~SerialLogger() = default;
    void logInfo(const char* message) override;
    void logWarning(const char* message) override;
    void logError(const char* message) override;
};