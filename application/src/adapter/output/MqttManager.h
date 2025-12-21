#pragma once
#include <PubSubClient.h>
#include <WiFi.h>
#include <Arduino.h>
#include <adapter/output/Logger.h>
#include <functional>
#include "queue.h"

#define MQTT_SERVER "homeassistant.local"
#define MQTT_PORT 1883
#define MQTT_ID "ats-thiaged"
#define MQTT_USER "mqtt_user"
#define MQTT_PASSWORD "123@123"
#define LOG_MQTT_TOPIC "casa/logs/message"

struct MQTTMessage {
    char topic[128];
    char payload[256];
    bool retained;
};

class MQTTManager {
private:
    using MessageCallback = std::function<void(char*, byte*, unsigned int)>;
    MessageCallback callback;
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    Logger &logger;

    boolean initialized = false;

    // MQTT reconnection helpers
    unsigned long lastMqttReconnectAttempt;
    unsigned long mqttReconnectIntervalMs;

    void attemptMqttReconnect();
    void subscribeAllMqttTopics();

    QueueHandle_t mqttQueue;
    TaskHandle_t mqttTaskHandle;
    void funcMqttTask();
    static void mqttTask(void *pvParameters);
public:
    MQTTManager(Logger &logger);

    bool connect();
    void loop();
    bool publish(const char* topic, const char* payload, bool retained = false);
    void subscribe(const char* topic);
    void setCallback(MessageCallback cb);
    bool connected();

    void StartMqttTask();
    void Init();

};