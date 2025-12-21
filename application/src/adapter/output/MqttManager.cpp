#include <adapter/output/MqttManager.h>

MQTTManager::MQTTManager(Logger &logger)
    : logger(logger), mqttClient(wifiClient)
{
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback([this](char* topic, byte* payload, unsigned int len) {
        if (callback) callback(topic, payload, len);
    });

    // init mqtt reconnect helpers
    lastMqttReconnectAttempt = 0;
    mqttReconnectIntervalMs = 5000; // try every 5s
}

bool MQTTManager::connect() {
    if (!WiFi.isConnected()) return false;
    if (mqttClient.connected()) return true;

    return mqttClient.connect(MQTT_ID, MQTT_USER, MQTT_PASSWORD);
}

void MQTTManager::loop() {
    mqttClient.loop();
    // If disconnected and WiFi is available, try reconnecting occasionally
    if (!mqttClient.connected())
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            attemptMqttReconnect();
        }
    }
}

// Em MQTTManager.cpp
bool MQTTManager::publish(const char* topic, const char* payload, bool retained)
{
    if (!topic || !payload) {
        return false;
    }

    MQTTMessage msg = {};

    strncpy(msg.topic, topic, sizeof(msg.topic) - 1);
    msg.topic[sizeof(msg.topic) - 1] = '\0';

    strncpy(msg.payload, payload, sizeof(msg.payload) - 1);
    msg.payload[sizeof(msg.payload) - 1] = '\0';

    msg.retained = retained;

    if (xQueueSend(mqttQueue, &msg, 0) != pdTRUE) {
        return false;
    }

    return true;
}

void MQTTManager::subscribe(const char *topic)
{
    if (connected()) {
        mqttClient.subscribe(topic);
    }
}

void MQTTManager::setCallback(MessageCallback cb)
{
    callback = cb;
}

bool MQTTManager::connected()
{
    return mqttClient.connected();
}

void MQTTManager::attemptMqttReconnect()
{
    unsigned long now = millis();
    if (now - lastMqttReconnectAttempt < mqttReconnectIntervalMs)
    {
        return; // respect interval
    }

    lastMqttReconnectAttempt = now;

    logger.logInfo("Tentando reconectar ao MQTT...");
    if (mqttClient.connect(MQTT_ID, MQTT_USER, MQTT_PASSWORD))
    {
        logger.logInfo("Reconectado ao MQTT");
        subscribeAllMqttTopics();
    }
    else
    {
        logger.logError("Falha ao reconectar ao MQTT");
    }
}

void MQTTManager::subscribeAllMqttTopics()
{
    mqttClient.subscribe("casa/bms/set");
    mqttClient.subscribe("casa/power_source/set");
    mqttClient.subscribe("casa/locked_to_source/set");
    mqttClient.subscribe("casa/wait_for_sync/set");
    mqttClient.subscribe("casa/voltage_calibration_sol/set");
    mqttClient.subscribe("casa/voltage_calibration_uti/set");
    mqttClient.subscribe("casa/voltage_offset_utility/set");
    mqttClient.subscribe("casa/voltage_offset_solar/set");
    mqttClient.subscribe("casa/firmware_utility_check/set");
    mqttClient.subscribe("casa/battery_voltage_solar/set");
    mqttClient.subscribe("casa/battery_voltage_utility/set");
    mqttClient.subscribe("casa/battery_percentage_solar/set");
    mqttClient.subscribe("casa/battery_percentage_utility/set");
}

void MQTTManager::funcMqttTask()
{
    while (true)
    {
        if (!initialized) {
            vTaskDelay(50 / portTICK_PERIOD_MS);
            continue;
        }

        if (WiFi.status() != WL_CONNECTED) {
            // WiFi não conectado, descarta a mensagem
            vTaskDelay(50 / portTICK_PERIOD_MS);
            continue;
        }

        if (!connected())
        {
            vTaskDelay(50 / portTICK_PERIOD_MS);
            continue;
        }

        MQTTMessage msg;
        if (xQueueReceive(mqttQueue, &msg, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (mqttClient.connected()) {
                mqttClient.publish(msg.topic, msg.payload, msg.retained);
            }
        }

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void MQTTManager::mqttTask(void *pvParameters)
{
    MQTTManager *instance = static_cast<MQTTManager *>(pvParameters);
    instance->funcMqttTask();
}

void MQTTManager::StartMqttTask()
{
    // Cria a fila com capacidade para 100 mensagens de log
    mqttQueue = xQueueCreate(30, sizeof(MQTTMessage));

    // Inicia a tarefa de log
    xTaskCreatePinnedToCore(
        mqttTask,
        "MqttTask",
        4096,
        this,
        4,
        &mqttTaskHandle,
        1 // Core 1
    );
}

void MQTTManager::Init()
{
    initialized = true;
}