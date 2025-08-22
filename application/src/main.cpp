#include <LilyGo_AMOLED.h>
#include <TFT_eSPI.h>
// #include <WiFiClient.h>
// #include <WiFi.h>
#include <WiFiManager.h>
#include "time.h"
#include <adapter/output/ScreenManager.h>
#include <adapter/output/ScreenHome.h>
#include <adapter/output/ScreenWave.h>
#include <adapter/output/ScreenClock.h>
#include <core/domain/SolarSource.h>
#include <core/domain/UtilitySource.h>
#include <memory>
#include <adapter/input/EnergyInput.h>
#include <core/domain/TransferManager.h>
#include <adapter/output/BuzzerManager.h>
#include "adapter/input/WebServerManager.h"

const int cyclesToCapture = 3;
const int samplesPerCycle = 80;
int totalSamples = samplesPerCycle * cyclesToCapture;
const float sampleInterval = 1.0 / FREQUENCY * 1000000 / samplesPerCycle; // Intervalo entre amostras para 60Hz em microsegundos

LilyGo_Class amoled;
#define WIDTH amoled.width()
#define HEIGHT amoled.height()

#define ZMPT_SOLAR_PIN 1
#define ZMPT_UTILITY_PIN 2
#define BUZZER_PIN 10
#define SOLAR_BUTTON_PIN 42
#define MENU_BUTTON_PIN 45
#define UTILITY_BUTTON_PIN 46

#define LONG_PRESS_SOLAR_DURATION 3000 // Duração para long press em milissegundos
#define LONG_PRESS_UTILITY_DURATION 3000 // Duração para long press em milissegundos

unsigned long inactivityTime = 0; // Tempo de inatividade
unsigned long pressSolarStartTime = 0; // Tempo de início da pressão
bool isLongPressSolar = false; // Flag para indicar long press
unsigned long pressUtilityStartTime = 0; // Tempo de início da pressão
bool isLongPressUtility = false; // Flag para indicar long press
bool updatingFirmware = false;
bool waitForSync = true;
bool inSleepMode = false;
bool firmwareUpdateUtilityOn = true;

uint32_t dcOffsetBattery = 0; // Valor de offset DC
uint32_t dcOffsetUtility = 1940; // Valor de offset DC
uint32_t dcOffsetSolar = 1940; // Valor de offset DC

uint8_t sourceLockResetHour = 8;

TFT_eSPI display = TFT_eSPI();
TFT_eSprite *systemSprite = new TFT_eSprite(&display);

// Connect to WiFi
WiFiManager wm;

Preferences configPreferences;

ScreenManager screenManager = ScreenManager();
DynamicAnalogBuffer batteryBufferService = DynamicAnalogBuffer(20, dcOffsetBattery);
DynamicAnalogBuffer solarBufferService = DynamicAnalogBuffer(totalSamples, dcOffsetSolar);
DynamicAnalogBuffer utilityBufferService = DynamicAnalogBuffer(totalSamples, dcOffsetUtility);

BatterySource battery = BatterySource(batteryBufferService, configPreferences, Serial2, &updatingFirmware);
DrawManager drawManager = DrawManager(amoled, inSleepMode);
UtilitySource utilitySource = UtilitySource(configPreferences, utilityBufferService, SourceType::UTILITY_SOURCE_TYPE, ZMPT_UTILITY_PIN);
SolarSource solarSource = SolarSource(configPreferences, solarBufferService, SourceType::SOLAR_SOURCE_TYPE, ZMPT_SOLAR_PIN);
EnergySource *activeSource = &utilitySource;
EnergySource *definedSource = &utilitySource;
EnergySource *userDefinedSource = NULL;

bool userSourceLocked = false;

ScreenHome screenHome = ScreenHome(utilitySource, solarSource, &activeSource, &definedSource, &userDefinedSource, battery,
    drawManager, display, amoled, &userSourceLocked, configPreferences);
ScreenWave screenWave = ScreenWave(utilitySource, solarSource, &activeSource, &definedSource, &userDefinedSource, battery,
    drawManager, display, amoled, cyclesToCapture, samplesPerCycle, &userSourceLocked, configPreferences);
ScreenClock screenClock = ScreenClock(utilitySource, solarSource, &activeSource, &definedSource, &userDefinedSource, battery,
    drawManager, display, amoled, &userSourceLocked, configPreferences);
ScreenUpdate screenUpdate = ScreenUpdate(utilitySource, solarSource, &activeSource, &definedSource, &userDefinedSource, battery,
    drawManager, display, amoled, &userSourceLocked, configPreferences);

WebserverManager serverManager = WebserverManager(utilitySource, solarSource, battery, screenManager, screenUpdate, &userDefinedSource,
    &userSourceLocked, &updatingFirmware, waitForSync, configPreferences, inactivityTime, firmwareUpdateUtilityOn);

EnergyInput energyInput = EnergyInput(utilitySource, solarSource, screenWave, sampleInterval, &updatingFirmware);

BuzzerManager buzzerManager = BuzzerManager(BUZZER_PIN);
TransferManager transferManager = TransferManager(&activeSource, &definedSource, &userDefinedSource, energyInput, screenManager,
    screenWave, buzzerManager, battery, &userSourceLocked, &updatingFirmware, &inactivityTime, waitForSync);

// NTP server settings
const char *ntpServer = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const long gmtOffset_sec = -10800; // São Paulo timezone (UTC-3)
const int daylightOffset_sec = 0;  // No daylight saving time

char timeHour[3];
char timeMin[3];
char timeSec[3];

unsigned long btnDebunceSolar = 0;
unsigned long btnDebunceUtility = 0;
unsigned long btnDebunceMenu = 0;
unsigned long btnDebunceBacklight = 0;

unsigned long debounceNotify = 0;

unsigned long ledBlinkTimer = 0;
bool ledBlink = 0;

void checkFailures();

void setup() {

    bool initiatedBoard = false;
    Serial.begin(115200);
    Serial.println("Starting...");
    delay(2000);

    // Inicializa a tela
    initiatedBoard = amoled.beginAMOLED_191(false);
    if (!initiatedBoard)
    {
        Serial.println("Failed to initialize the board");
        while (true)
            delay(1000);
    }

    amoled.setRotation(0);

    pinMode(SOLAR_BUTTON_PIN, INPUT_PULLUP);
    pinMode(MENU_BUTTON_PIN, INPUT_PULLUP);
    pinMode(UTILITY_BUTTON_PIN, INPUT_PULLUP);
    pinMode(21, INPUT_PULLUP);

    drawManager.StartDrawTask();
    checkFailures();

    battery.Init();
    energyInput.Init();
    transferManager.Init();
    buzzerManager.Init();

    systemSprite->createSprite(WIDTH, HEIGHT);
    systemSprite->fillSprite(BG_SYSTEM);
    drawManager.RequestDraw(0, 0, WIDTH, HEIGHT, systemSprite, nullptr, nullptr);

    systemSprite->createSprite(300, 30); // Criar o sprite com 300x30 pixels
    systemSprite->fillSprite(BG_SYSTEM); // Preencher o fundo do sprite
    systemSprite->drawString("Wi-fi connecting...", 0, 0, 4);
    drawManager.RequestDraw(10, 4, 300, 30, systemSprite, nullptr, nullptr);

    bool res;
    res = wm.autoConnect("ATS_thiaged", "123@123"); // password protected ap

    if (res)
    {
        while (WiFi.status() != WL_CONNECTED) {
            Serial.print(".");
            delay(500);
        }

        // if you get here you have connected to the WiFi
        Serial.println("connected to wifi :)");
        // Initialize and configure NTP
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer, ntpServer2);

        systemSprite->createSprite(WIDTH, HEIGHT);
        systemSprite->fillSprite(BG_SYSTEM);
        drawManager.RequestDraw(0, 0, WIDTH, HEIGHT, systemSprite, nullptr, nullptr);

        serverManager.Init();
    }

    // Adiciona telas
    screenManager.AddScreen(std::unique_ptr<ScreenHome>(&screenHome)); //add tela home como primeira tela
    screenManager.AddScreen(std::unique_ptr<ScreenWave>(&screenWave));
    screenManager.AddScreen(std::unique_ptr<ScreenClock>(&screenClock));
    screenManager.AddScreen(std::unique_ptr<ScreenUpdate>(&screenUpdate));

    buzzerManager.PlayInitSound();

    screenManager.Render();
    inactivityTime = millis();

}

void loop() {

    if (digitalRead(21) == LOW)
    {
        if ((millis() - btnDebunceBacklight) > 300) {
            btnDebunceBacklight = millis();
            buzzerManager.PlayClickSound();
            inactivityTime = millis();
        }
    }

    if (digitalRead(SOLAR_BUTTON_PIN) == LOW)
    {
        inactivityTime = millis();
        if ((millis() - btnDebunceSolar) > 300) {
            btnDebunceSolar = millis();

            buzzerManager.PlayClickSound();
            userSourceLocked = true;

            if (userDefinedSource != &solarSource) {
                userDefinedSource = &solarSource;
            } else {
                userSourceLocked = false;
                userDefinedSource = nullptr;
            }

        }

    }

    if (digitalRead(UTILITY_BUTTON_PIN) == LOW)
    {
        inactivityTime = millis();
        if ((millis() - btnDebunceUtility) > 300) {
            btnDebunceUtility = millis();

            buzzerManager.PlayClickSound();
            userSourceLocked = true;

            if (userDefinedSource != &utilitySource) {
                userDefinedSource = &utilitySource;
            } else {
                userSourceLocked = false;
                userDefinedSource = nullptr;
            }

        }

    }

    if (digitalRead(MENU_BUTTON_PIN) == LOW && millis() - btnDebunceMenu > 300)
    {
        inactivityTime = millis();
        buzzerManager.PlayClickSound();
        btnDebunceMenu = millis();
        screenManager.NextScreen();
    }

    if (utilitySource.GetTension() < 90 || utilitySource.GetTension() > 140)
    {
        if ((millis() - ledBlinkTimer) > 500)
        {
            ledBlinkTimer = millis();
            ledBlink = !ledBlink;
            digitalWrite(UTILITY_OFF_LED_PIN, ledBlink);
        }
    } else {
        digitalWrite(UTILITY_OFF_LED_PIN, LOW);
    }

    screenManager.Render();

    // Auto-calibration for better AC tension stability
    if (!utilitySource.IsCalibrated()) {
        utilitySource.AutoCalibrateDcOffset();
    }
    if (!solarSource.IsCalibrated()) {
        solarSource.AutoCalibrateDcOffset();
    }

    if ((millis() - debounceNotify) > 5000)
    {

        Serial.println("BMS soc: " + String(battery.GetBms().soc));
        Serial.println("BMS voltage: " + String(battery.GetBms().totalVoltage));
        debounceNotify = millis();
        String bmsStatus = battery.GetBmsComunicationOn() ? "active" : "inactive";
        
        // Debug output for AC tension stability monitoring
        Serial.println("=== AC Tension Stability Debug ===");
        Serial.println("Utility - Voltage: " + String(utilitySource.GetTension(), 2) + 
                      "V, ADC: " + String(utilitySource.GetSensorValue()) + 
                      ", Calibrated: " + (utilitySource.IsCalibrated() ? "Yes" : "No"));
        Serial.println("Solar - Voltage: " + String(solarSource.GetTension(), 2) + 
                      "V, ADC: " + String(solarSource.GetSensorValue()) + 
                      ", Calibrated: " + (solarSource.IsCalibrated() ? "Yes" : "No"));
        Serial.println("==================================");
        
        if (serverManager.HasWsClients())
        {
            debounceNotify = millis();
            String data = "{\"solarVoltage\": \"" + String(solarSource.GetTension()) +
                          "\", \"solarStatus\": \"" + solarSource.GetStatusPrintable() +
                          "\", \"utilityVoltage\": \"" + String(utilitySource.GetTension()) +
                          "\", \"utilityStatus\": \"" + utilitySource.GetStatusPrintable() +
                          "\", \"usingSource\": \"" + activeSource->GetTypePrintable() +
                          "\", \"batteryVoltage\": \"" + String(battery.GetBatteryVoltage()) +
                          "\", \"statusBms\": \"" + bmsStatus +
                          "\", \"solarTopAdcValue\": \"" + String(solarSource.GetTopAdcValue()) +
                          "\", \"solarBottomAdcValue\": \"" + String(solarSource.GetBottomAdcValue()) +
                          "\", \"utilityTopAdcValue\": \"" + String(utilitySource.GetTopAdcValue()) +
                          "\", \"utilityBottomAdcValue\": \"" + String(utilitySource.GetBottomAdcValue()) +
                          "\"}";

            serverManager.NotifyClients(data);
        }

        serverManager.SendToMqtt("casa/solar/voltage", String(solarSource.GetTension()).c_str());
        serverManager.SendToMqtt("casa/solar/status", solarSource.GetStatusPrintable().c_str());
        serverManager.SendToMqtt("casa/utility/voltage", String(utilitySource.GetTension()).c_str());
        serverManager.SendToMqtt("casa/utility/status", utilitySource.GetStatusPrintable().c_str());
        serverManager.SendToMqtt("casa/battery/voltage", String(battery.GetBatteryVoltage()).c_str());
        serverManager.SendToMqtt("casa/bms/status", bmsStatus.c_str());
        serverManager.SendToMqtt("casa/power_source/status", activeSource->GetTypePrintable().c_str());
        serverManager.SendToMqtt("casa/locked_to_source/status", userSourceLocked ? "active" : "inactive");
        serverManager.SendToMqtt("casa/wait_for_sync/status", waitForSync ? "active" : "inactive");
        serverManager.SendToMqtt("casa/voltage_calibration_uti/status", String(utilitySource.GetVoltageCalibration(), 4).c_str());
        serverManager.SendToMqtt("casa/voltage_calibration_sol/status", String(solarSource.GetVoltageCalibration(), 4).c_str());
        serverManager.SendToMqtt("casa/voltage_offset_utility/status", String(dcOffsetUtility).c_str());
        serverManager.SendToMqtt("casa/voltage_offset_solar/status", String(dcOffsetSolar).c_str());
        serverManager.SendToMqtt("casa/firmware_utility_check/status", firmwareUpdateUtilityOn ? "active" : "inactive");
        serverManager.SendToMqtt("casa/battery_voltage_solar/status", String(battery.GetBatteryConfigMax()).c_str());
        serverManager.SendToMqtt("casa/battery_voltage_utility/status", String(battery.GetBatteryConfigMin()).c_str());
        serverManager.SendToMqtt("casa/battery_percentage_solar/status", String(battery.GetBatteryConfigMaxPercentage()).c_str());
        serverManager.SendToMqtt("casa/battery_percentage_utility/status", String(battery.GetBatteryConfigMinPercentage()).c_str());

        if ((millis() - inactivityTime) > 300000)
        {
            if (amoled.getBrightness() > 0) {
                amoled.setBrightness(0);
            }

        } else {
            if (amoled.getBrightness() == 0) {
                amoled.setBrightness(AMOLED_DEFAULT_BRIGHTNESS);
            }
        }
    }

    serverManager.LoopMqtt();

    delay(60);
}

void checkFailures()
{
    esp_reset_reason_t resetReason = esp_reset_reason();
    bool isCrash = false;
    String reason = "";

    switch (resetReason)
    {
    case ESP_RST_POWERON:
        reason = "Power On";
        break;
    case ESP_RST_EXT:
        reason = "Reset Externo (pino EN ou botão de reset)";
        break;
    case ESP_RST_SW:
        reason = "Reset por Software (esp_restart())";
        break;
    case ESP_RST_PANIC:
        reason = "Crash (Kernel Panic, Exception)";
        isCrash = true;
        break;
    case ESP_RST_INT_WDT:
        reason = "Watchdog interno disparou";
        isCrash = true;
        break;
    case ESP_RST_TASK_WDT:
        reason = "Watchdog de task disparou";
        isCrash = true;
        break;
    case ESP_RST_WDT:
        reason = "Outro Watchdog reiniciou";
        isCrash = true;
        break;
    case ESP_RST_DEEPSLEEP:
        reason = "Acordou do Deep Sleep";
        break;
    case ESP_RST_BROWNOUT:
        reason = "Queda de tensão (Brownout)";
        break;
    case ESP_RST_SDIO:
        reason = "Reset por SDIO";
        break;
    default:
        reason = "Desconhecido";
    }

    Serial.println("Reset reason: " + reason);

    if (isCrash)
    {
        systemSprite->createSprite(WIDTH, HEIGHT);
        systemSprite->fillSprite(BG_SYSTEM);
        systemSprite->drawString("Checking reset...", 0, 0, 4);
        systemSprite->drawString(reason, 0, 40, 4);
        drawManager.RequestDraw(0, 0, WIDTH, HEIGHT, systemSprite, nullptr, nullptr);

        ledcSetup(0, 3000, 8);  // Canal 0, frequência 2000Hz, resolução de 8 bits
        ledcAttachPin(BUZZER_PIN, 0);

        bool wait = true;
        uint32_t elapsedTime = millis();

        while (wait)
        {

            if ((millis() - elapsedTime) > 300000)
            {
                ledcWrite(0, 153);  // Inicializa o buzzer com 50% de duty cycle
                delay(300);
                ledcWriteTone(0, 0);  // Desliga o buzzer
            }

            ledBlink = !ledBlink;
            digitalWrite(UTILITY_OFF_LED_PIN, ledBlink);

            wait = digitalRead(MENU_BUTTON_PIN) == HIGH;
            delay(200);
        }

    }

}
