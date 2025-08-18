#pragma once
#include <Arduino.h>
#include <esp_timer.h>
#include <HardwareSerial.h>
#include "DynamicAnalogBuffer.h"
#include <Preferences.h>

#define BATTERY_SENSOR_R1 100000.0
#define BATTERY_SENSOR_R2 10000.0
#define BATTERY_SENSOR_PIN 3

struct BatteryConfig {
    float batteryCalibrationValue;
};

const BatteryConfig defaultBatteryConfig = {
    1.634
};

#define RS485_RX 44
#define RS485_TX 43
#define RS485_BAUD 9600

// Comandos para comunicação com a BMS JK
// Comando para solicitar informações básicas da BMS
const byte REQUEST_BASIC_INFO[] = {0xDD, 0xA5, 0x03, 0x00, 0xFF, 0xFD, 0x77};
// Tamanho do comando
const int REQUEST_BASIC_INFO_SIZE = sizeof(REQUEST_BASIC_INFO);

const int MAX_BUFFER_SIZE = 128;

// Estrutura para armazenar os dados da BMS
struct BMSData {
  float totalVoltage = 0.0;
  float current = 0.0;
  float power = 0.0;
  float temperature = 0.0;
  int soc = 0; // State of Charge (%)
  int cellCount = 0;
  float cellVoltages[8] = {0}; // Suporte para até 8 células
  bool isConnected = false;
  unsigned long lastResponseTime = 0;
};

class BatterySource
{
private:
    Preferences &configPreferences;
    DynamicAnalogBuffer &batteryReadBuffer;
    BatteryConfig batteryConfig = defaultBatteryConfig;
    double batteryConfigMin = 23.50;               // switch to utility value
    unsigned int batteryConfigMinPercentage = 40; // switch to utility value
    unsigned int batteryConfigMaxPercentage = 60; // switch to solar value
    double batteryConfigMax = 27.00;               // switch to solar value
    double batteryVoltage = 0.00;                 // current battery voltage
    unsigned int batteryPercentage = 0;         // current battery percentage
    double batteryVoltageMin = 20.00;          // voltage for 0%
    double batteryVoltageMax = 29.00;          // voltage for 100%
    const uint64_t peakSurgeDelay = 10000000; // atrasar recuperacao da rede, valor em microsegundos. 1.000.000 = 1s
    uint64_t lowBatteryStarted = 0;
    const int sampleIntervalBattery = 30000; // microsegundos
    bool bmsComunicationOn = false;
    unsigned int bmsComunicationFailures = 0;
    // esp_timer_handle_t timer_sensor_battery; // TODO: remover timer
    HardwareSerial& RS485Serial;
    bool *updatingFirmware;

    BMSData BMS;

    TaskHandle_t batterySensorTaskHandle = NULL;

    int bufferIndex = 0;
    byte receiveBuffer[MAX_BUFFER_SIZE];
    unsigned long lastBmsRequestTime = 0;

    void funcSensorReadTask();
    static void sensorReadTask(void *pvParameters);
    void startSensorReadTask();
    void readBatteryResistorDivisor();
    void readBatteryBMS();

    void processReceivedData();
    void sendCommand(const byte* command, int length);
    void checkConnectionTimeout();

public:
    BatterySource(
        DynamicAnalogBuffer &pBatteryReadBuffer,
        Preferences &pConfigPreferences,
        HardwareSerial &pSerial,
        bool *pUpdatingFirmware
    );
    virtual ~BatterySource() = default;
    void Init();

    double GetBatteryConfigMin();
    unsigned int GetBatteryConfigMinPercentage();
    unsigned int GetBatteryConfigMaxPercentage();
    double GetBatteryConfigMax();
    double GetBatteryVoltage();
    double GetBatteryPercentage();
    double GetBatteryVoltageMin();
    double GetBatteryVoltageMax();
    uint64_t GetLowBatStarted();
    uint64_t GetPeakSurgeDelay();
    bool GetBmsComunicationOn();
    BMSData GetBms();

    bool IsHighBattery();

    bool IsLowBattery();
    bool IsEmpty(); // when tension value is under min value

    void SetBatteryVoltage(double value);
    void SetBatteryPercentage(double value);
    void SetBmsComunicationOn(bool value);
    void SetBatteryConfigMin(double value);
    void SetBatteryConfigMax(double value);
    void SetBatteryConfigMinPercentage(int value);
    void SetBatteryConfigMaxPercentage(int value);
    void SetBatteryVoltageMin(double value);
    void SetBatteryVoltageMax(double value);

    void UpdateLowBatteryStarted();

    bool LowBatteryDelayEnded();
    void ResetLowbatteryStarted();
};