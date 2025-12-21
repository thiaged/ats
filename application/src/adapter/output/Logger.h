#pragma once
#include <cstdarg>
#include <cstdio>

enum LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
private:

protected:
    LogLevel logLevel = LogLevel::INFO;
public:
    Logger(LogLevel level = LogLevel::INFO);
    virtual ~Logger() = default;

    virtual void logInfo(const char* message) = 0;
    void logInfoF(const char* format, ...);
    virtual void logWarning(const char* message) = 0;
    void logWarningF(const char* format, ...);
    virtual void logError(const char* message) = 0;
    void logErrorF(const char* format, ...);
    void setLogLevel(LogLevel newLevel);
};