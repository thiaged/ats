#include "Logger.h"

Logger::Logger(LogLevel level)
{
    logLevel = level;
}

void Logger::logInfoF(const char *format, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    logInfo(buffer);
}

void Logger::logWarningF(const char *format, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    logWarning(buffer);
}

void Logger::logErrorF(const char *format, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    logError(buffer);
}

void Logger::setLogLevel(LogLevel newLevel)
{
    logLevel = newLevel;
}