#include "WebServerManager.h"
#include <ArduinoJson.h>
#include <driver/rtc_io.h>

String WebserverManager::setMenuEnabled(String html, MenuList menu)
{
    html.replace("%CSS_HTML%", css_html);
    html.replace("%MENU%", menu_html);
    switch (menu)
    {
    case MenuList::HOME:
        html.replace("%CLASS_HOME_ACTIVE%", "active");
        html.replace("%CLASS_PREFERENCE_ACTIVE%", "");
        html.replace("%CLASS_SOBRE_ACTIVE%", "");
        return html;
        break;

    case MenuList::PREFERENCE:
        html.replace("%CLASS_HOME_ACTIVE%", "");
        html.replace("%CLASS_PREFERENCE_ACTIVE%", "active");
        html.replace("%CLASS_SOBRE_ACTIVE%", "");
        return html;
        break;

    case MenuList::ABOUT:
        html.replace("%CLASS_HOME_ACTIVE%", "");
        html.replace("%CLASS_PREFERENCE_ACTIVE%", "");
        html.replace("%CLASS_SOBRE_ACTIVE%", "active");
        return html;
        break;

    default:
        break;
    }
    return String();
}

void WebserverManager::onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        Serial.println("Cliente WebSocket conectado");
    }
    else if (type == WS_EVT_DATA)
    {
        String msg = String(reinterpret_cast<char *>(data), len);
        Serial.println("Dados recebidos: " + msg);
    }
}

void WebserverManager::onMqttMessage(char *topic, byte *payload, unsigned int length)
{
    String message = String((char *)payload).substring(0, length);
    Serial.println("Mensagem recebida no tópico " + String(topic) + ": " + message);

    if (String(topic) == "casa/bms/set")
    {
        // Aqui você pode processar a mensagem recebida
        // Por exemplo, se a mensagem for "active", ative o BMS
        if (message == "active")
        {
            battery.SetBmsComunicationOn(true);
            Serial.println("BMS ativado");
        }
        else if (message == "inactive")
        {
            battery.SetBmsComunicationOn(false);
            Serial.println("BMS desativado");
        }
    }

    if (String(topic) == "casa/power_source/set")
    {
        // Aqui você pode processar a mensagem recebida
        // Por exemplo, se a mensagem for "solar", ative o BMS
        if (message == "solar")
        {
            *userDefinedSource = &solarSource;
            *userSourceLocked = true;
            Serial.println("User mudou para solar");
        }
        else if (message == "utility")
        {
            *userDefinedSource = &utilitySource;
            *userSourceLocked = true;
            Serial.println("User mudou para utility");
        }
    }

    if (String(topic) == "casa/locked_to_source/set")
    {
        // Aqui você pode processar a mensagem recebida
        // Por exemplo, se a mensagem for "solar", ative o BMS
        if (message == "inactive")
        {
            *userSourceLocked = false;
            *userDefinedSource = nullptr;
            Serial.println("User destravou a transferência");
        }
    }

    if (String(topic) == "casa/wait_for_sync/set")
    {
        if (message == "active")
        {
            SetWaitForSync(true);
            Serial.println("Aguarda sincronização");
        }
        else if (message == "inactive")
        {
            SetWaitForSync(false);
            Serial.println("Não aguarda sincronização");
        }
    }

    if (String(topic) == "casa/firmware_utility_check/set")
    {
        if (message == "active")
        {
            firmwareUpdateUtilityOn = true;
        }
        else if (message == "inactive")
        {
            firmwareUpdateUtilityOn = false;
        }
    }

    if (String(topic) == "casa/voltage_calibration_sol/set")
    {
        solarSource.SetVoltageCalibration(message.toDouble());
    }

    if (String(topic) == "casa/voltage_calibration_uti/set")
    {
        utilitySource.SetVoltageCalibration(message.toDouble());
    }

    if (String(topic) == "casa/voltage_offset_solar/set")
    {
        solarSource.SetVoltageOffset(message.toInt());
    }

    if (String(topic) == "casa/voltage_offset_utility/set")
    {
        utilitySource.SetVoltageOffset(message.toInt());
    }

    if (String(topic) == "casa/battery_voltage_solar/set")
    {
        battery.SetBatteryConfigMax(message.toDouble());
    }

    if (String(topic) == "casa/battery_voltage_utility/set")
    {
        battery.SetBatteryConfigMin(message.toDouble());
    }

    if (String(topic) == "casa/battery_percentage_solar/set")
    {
        battery.SetBatteryConfigMaxPercentage(message.toInt());
    }

    if (String(topic) == "casa/battery_percentage_utility/set")
    {
        battery.SetBatteryConfigMinPercentage(message.toInt());
    }

}

void WebserverManager::subscribeAllMqttTopics()
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

void WebserverManager::NotifyClients(const String &message)
{
    websocket.textAll(message);
}

void WebserverManager::NotifyClientsEvent(const char *message, const char *eventName)
{
    events.send(message, eventName, millis());
}

bool WebserverManager::HasWsClients()
{
    return websocket.count() > 0;
}

void WebserverManager::SendToMqtt(const char *topic, const char *message)
{
    if (!mqttClient.connected())
    {
        mqttClient.connect(MQTT_ID, MQTT_USER, MQTT_PASSWORD);
        if (!mqttClient.connected())
        {
            Serial.println("Falha ao conectar ao MQTT");
            return;
        }
        subscribeAllMqttTopics();
    }
    mqttClient.publish(topic, message, true);
}

void WebserverManager::LoopMqtt()
{
    mqttClient.loop();
}

void WebserverManager::SetWaitForSync(bool wait)
{
    bool changed = wait != waitForSync;
    waitForSync = wait;

    if (changed)
    {
        configPreferences.begin("config", false);
        configPreferences.putBool("wait-sync", wait); //maximo 15 char on name
        configPreferences.end();
    }
}

WebserverManager::WebserverManager(
    EnergySource &pUtilitySource,
    EnergySource &pSolarSource,
    BatterySource &pBattery,
    ScreenManager &pScreenManager,
    ScreenUpdate &pScreenUpdate,
    EnergySource **pUserDefinedSource,
    bool *pUserSourceLocked,
    bool *pUpdatingFirmware,
    bool &pWaitForSync,
    Preferences &pConfigPreferences,
    unsigned long &pInactivityTime,
    bool &pFirmwareUpdateUtilityOn
)
    : utilitySource(pUtilitySource), solarSource(pSolarSource), battery(pBattery), screenManager(pScreenManager),
      screenUpdate(pScreenUpdate), server(80), websocket("/ws"), events("/events"), mqttClient(client), waitForSync(pWaitForSync),
      configPreferences(pConfigPreferences), inactivityTime(pInactivityTime), firmwareUpdateUtilityOn(pFirmwareUpdateUtilityOn)
{
    userDefinedSource = pUserDefinedSource;
    userSourceLocked = pUserSourceLocked;
    updatingFirmware = pUpdatingFirmware;
}

void WebserverManager::Init()
{
    configPreferences.begin("config", true);
    waitForSync = configPreferences.getBool("wait-sync", true);

    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
        String html = String(index_html);
        html = setMenuEnabled(html, MenuList::HOME);

        html.replace("%solarStatus%", solarSource.GetStatusPrintable());
        html.replace("%utilityStatus%", utilitySource.GetStatusPrintable());
        html.replace("%solarVoltage%", String(solarSource.GetTension()) + "V");
        html.replace("%utilityVoltage%", String(utilitySource.GetTension()) + "V");
        html.replace("%batteryVoltage%", String(battery.GetBatteryVoltage()) + "V");
        request->send(200, "text/html", html); });

    server.on("/preference", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
        String html = String(preference_html);
        html = setMenuEnabled(html, MenuList::PREFERENCE);

        html.replace("%BMSCONFIGSELECTOFF%", !battery.GetBmsComunicationOn() ? "selected" : "");
        html.replace("%BMSCONFIGSELECTON%", battery.GetBmsComunicationOn() ? "selected" : "");

        html.replace("%check_wait_sinc_status_on%", waitForSync ? "selected" : "");
        html.replace("%check_wait_sinc_status_off%", !waitForSync ? "selected" : "");

        html.replace("%batteryConfigMin%", String(battery.GetBatteryConfigMin()));
        html.replace("%batteryConfigMax%", String(battery.GetBatteryConfigMax()));

        html.replace("%batteryConfigMinPercentage%", String(battery.GetBatteryConfigMinPercentage()));
        html.replace("%batteryConfigMaxPercentage%", String(battery.GetBatteryConfigMaxPercentage()));

        html.replace("%batteryVoltageMin%", String(battery.GetBatteryVoltageMin()));
        html.replace("%batteryVoltageMax%", String(battery.GetBatteryVoltageMax()));

        html.replace("%calib-ac%", String(utilitySource.GetVoltageCalibration()));

        request->send(200, "text/html", html); });

    server.on("/preference", HTTP_POST, [this](AsyncWebServerRequest *request)
              {
        if (request->hasParam("waitSyncStatus", true)) {  // "true" indica que o dado vem do corpo da requisição
            String param = request->getParam("waitSyncStatus", true)->value();
            SetWaitForSync(param == "1");
        }

        if (request->hasParam("calib-ac", true)) {  // "true" indica que o dado vem do corpo da requisição
            String param = request->getParam("calib-ac", true)->value();
            utilitySource.SetVoltageCalibration(param.toDouble());
            solarSource.SetVoltageCalibration(param.toDouble());
        }

        if (request->hasParam("bmsStatus", true)) {  // "true" indica que o dado vem do corpo da requisição
            String param = request->getParam("bmsStatus", true)->value();
            battery.SetBmsComunicationOn(param == "1");
        }

        if (request->hasParam("batteryConfigMin", true)) {  // "true" indica que o dado vem do corpo da requisição
            String param = request->getParam("batteryConfigMin", true)->value();
            battery.SetBatteryConfigMin(param.toDouble());
        }

        if (request->hasParam("batteryConfigMax", true)) {  // "true" indica que o dado vem do corpo da requisição
            String param = request->getParam("batteryConfigMax", true)->value();
            battery.SetBatteryConfigMax(param.toDouble());
        }

        if (request->hasParam("batteryConfigMinPercentage", true)) {  // "true" indica que o dado vem do corpo da requisição
            String param = request->getParam("batteryConfigMinPercentage", true)->value();
            battery.SetBatteryConfigMinPercentage(param.toInt());
        }

        if (request->hasParam("batteryConfigMaxPercentage", true)) {  // "true" indica que o dado vem do corpo da requisição
            String param = request->getParam("batteryConfigMaxPercentage", true)->value();
            battery.SetBatteryConfigMaxPercentage(param.toInt());
        }

        if (request->hasParam("batteryVoltageMin", true)) {  // "true" indica que o dado vem do corpo da requisição
            String param = request->getParam("batteryVoltageMin", true)->value();
            battery.SetBatteryVoltageMin(param.toDouble());
        }

        if (request->hasParam("batteryVoltageMax", true)) {  // "true" indica que o dado vem do corpo da requisição
            String param = request->getParam("batteryVoltageMax", true)->value();
            battery.SetBatteryVoltageMax(param.toDouble());
        }

        String html = String(form_action_html);
        html = setMenuEnabled(html, MenuList::PREFERENCE);

        request->send(200, "text/html", html); });

    server.on("/about", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
        String html = String(about_html);
        html = setMenuEnabled(html, MenuList::ABOUT);

        request->send(200, "text/html", html); });

    server.on(
        "/about",
        HTTP_POST,
        [this](AsyncWebServerRequest *request)
        {
            // Responde ao cliente que o upload foi concluído
            String html = String(form_action_html);
            html = setMenuEnabled(html, MenuList::ABOUT);
            html.replace("Salvo com sucesso", "Atualização OTA concluída com sucesso! Reiniciando...");
            // esp_task_wdt_add(NULL); //watchdog
            request->send(200, "text/html", html);

            // rtc_gpio_hold_en((gpio_num_t)RELAY_PIN);
        },
        [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
        {
            // Função chamada durante o upload do arquivo

            if (!filename.endsWith(".bin"))
            {
                String html = String(form_action_html);
                html = setMenuEnabled(html, MenuList::ABOUT);
                html.replace("Salvo com sucesso", "Apenas arquivos .bin são permitidos!");
                request->send(500, "text/html", html);
                // Fecha a conexão para interromper qualquer transferência em andamento
                if (request->client())
                {
                    request->client()->close();
                }
                return;
            }

            inactivityTime = millis();

            if (!index)
            {
                // Inicia a atualização OTA no início do upload
                Serial.println("Iniciando upload...");

                if (firmwareUpdateUtilityOn && !utilitySource.IsOnline())
                {
                    String html = String(form_action_html);
                    html = setMenuEnabled(html, MenuList::ABOUT);
                    html.replace("Salvo com sucesso", "Não é possivel iniciar a atualização com a Cemig offline!");
                    request->send(500, "text/html", html);
                    // Fecha a conexão para interromper qualquer transferência em andamento
                    if (request->client())
                    {
                        request->client()->close();
                    }
                    return;
                }

                *updatingFirmware = true;

                unsigned long fileSize = request->contentLength();

                Serial.println(fileSize);
                // esp_task_wdt_delete(NULL); //wathdog
                screenUpdate.SetFirmwareSize(fileSize);
                screenManager.GoToScreen(&screenUpdate);

                if (!Update.begin(UPDATE_SIZE_UNKNOWN))
                {
                    Serial.println("Falha ao iniciar a atualização.");
                    String html = String(form_action_html);
                    html = setMenuEnabled(html, MenuList::ABOUT);
                    html.replace("Salvo com sucesso", "Falha ao iniciar a atualização!");
                    request->send(500, "text/html", html);
                    // Fecha a conexão para interromper qualquer transferência em andamento
                    if (request->client())
                    {
                        request->client()->close();
                    }
                    return;
                }
            }

            //   Escreve os dados recebidos na memória flash
            if (Update.write(data, len) != len)
            {
                Serial.println("Falha ao gravar dados na flash.");
                Update.abort();
                request->send(500, "text/html", "Falha ao gravar dados na flash.");
                // Fecha a conexão para interromper qualquer transferência em andamento
                if (request->client())
                {
                    request->client()->close();
                }
                return;
            }

            screenUpdate.IncreaseProcessedSize(len);

            if (final)
            {
                // Finaliza a atualização no fim do upload
                if (Update.end(true))
                {
                    Serial.printf("Atualização concluída: %u bytes\n", index + len);
                    screenUpdate.SetFirmwareSize(index + len);
                    screenUpdate.PrintProgress();
                    screenUpdate.SetRestart(true);
                }
                else
                {
                    Serial.println("Falha ao finalizar a atualização.");
                    Update.abort();
                    request->send(500, "text/html", "Falha ao finalizar a atualização.");
                    // Fecha a conexão para interromper qualquer transferência em andamento
                    if (request->client())
                    {
                        request->client()->close();
                    }
                }
            }
        });

    websocket.onEvent([&](AsyncWebSocket *s, AsyncWebSocketClient *c, AwsEventType type,
                          void *arg, uint8_t *data, size_t len)
                      { onWsEvent(s, c, type, arg, data, len); });
    server.addHandler(&websocket);

    events.onConnect([](AsyncEventSourceClient *client) {
        if(client->lastId()){
          Serial.printf("Client reconnected! Last message ID that it gat is: %u\n", client->lastId());
        }
        //send event with message "hello!", id current millis
        // and set reconnect delay to 1 second
        client->send("hello!", "open", millis(), 1000);
    });
    server.addHandler(&events);

    server.begin();

    // Configura o mDNS com o nome "meuesp.local"
    if (!MDNS.begin("atsthiaged"))
    {
        Serial.println("Erro ao iniciar mDNS");
        return;
    }
    Serial.println("mDNS iniciado com sucesso! Acesse: http://atsthiaged.local");

    mqttClient.setServer("homeassistant.local", 1883);
    mqttClient.setCallback([this](char *topic, byte *payload, unsigned int length)
                           { onMqttMessage(topic, payload, length); });
    mqttClient.connect(MQTT_ID, MQTT_USER, MQTT_PASSWORD);
    if (mqttClient.connected())
    {
        Serial.println("Conectado ao MQTT");
        subscribeAllMqttTopics();
    }
    else
    {
        Serial.println("Falha ao conectar ao MQTT");
    }

}
