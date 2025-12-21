#include <adapter/output/MqttLogger.h>
#include "MqttLogger.h"

MQTTLogger::MQTTLogger(LogLevel level, MQTTManager& pMqttManager)
    : Logger(level), mqttManager(pMqttManager)
{

}

void MQTTLogger::logInfo(const char *message)
{
    if (logLevel <= LogLevel::INFO)
    {
        mqttManager.publish(LOG_MQTT_TOPIC, message, false);
    }
}

void MQTTLogger::logWarning(const char *message)
{
    if (logLevel <= LogLevel::WARNING)
    {
        mqttManager.publish(LOG_MQTT_TOPIC, message, false);
    }
}

void MQTTLogger::logError(const char *message)
{
    if (logLevel <= LogLevel::ERROR)
    {
        mqttManager.publish(LOG_MQTT_TOPIC, message, false);
    }
}
