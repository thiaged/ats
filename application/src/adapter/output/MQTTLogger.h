#pragma once
#include <adapter/output/Logger.h>
#include <adapter/output/MqttManager.h>

struct LogMessage {
    LogLevel level;
    char message[256];
};

class MQTTLogger : public Logger {
private:
    MQTTManager& mqttManager;

public:
    MQTTLogger(LogLevel level, MQTTManager& pMqttManager);
    virtual ~MQTTLogger() = default;
    void logInfo(const char* message) override;
    void logWarning(const char* message) override;
    void logError(const char* message) override;

};