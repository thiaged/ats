#include "SerialLogger.h"

SerialLogger::SerialLogger(LogLevel level)
    : Logger(level)
{
}

void SerialLogger::logInfo(const char *message)
{
    Serial.println(message);
}

void SerialLogger::logWarning(const char *message)
{
    Serial.println(message);
}

void SerialLogger::logError(const char *message)
{
    Serial.println(message);
}
