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

const uint8_t MAX_CELLS = 20;
struct BmsData {
    uint8_t cellCount = 0;
    uint16_t cellVoltages[MAX_CELLS];       // in mV
    uint16_t powerTubeTemp = 0;             // code 0x80
    uint16_t boxTemp = 0;                   // code 0x81
    uint16_t batteryTemp = 0;               // code 0x82
    uint16_t totalVoltage = 0;              // code 0x83 (x0.01V)
    int16_t currentVal = 0;                 // code 0x84 (x0.01A)
    uint8_t soc = 0;                        // code 0x85 (%)
    uint8_t tempSensorCount = 0;            // code 0x86
    uint16_t cycleCount = 0;                // code 0x87
    uint32_t cycleCapacity = 0;             // code 0x89 (Ah)
    uint16_t stringCount = 0;               // code 0x8A
    uint16_t warningMask = 0;               // code 0x8B
    uint16_t statusMask = 0;                // code 0x8C
    uint8_t protocolVersion = 0;            // code 0xC0
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
    HardwareSerial& rs485 = Serial2;
    bool *updatingFirmware;

    BmsData BMS;

    TaskHandle_t batterySensorTaskHandle = NULL;

    void funcSensorReadTask();
    static void sensorReadTask(void *pvParameters);
    void startSensorReadTask();
    void readBatteryResistorDivisor();
    void readBatteryBMS();

    bool readAndStore();
    void sendReadRequest(const uint8_t *regs, size_t regCount);
    uint16_t calcCRC(const uint8_t *buf, size_t len);

public:
    BatterySource(
        DynamicAnalogBuffer &pBatteryReadBuffer,
        Preferences &pConfigPreferences,
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