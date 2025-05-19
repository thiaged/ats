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
    double batteryPercentage = 0.00;
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

    TaskHandle_t batterySensorTaskHandle = NULL;

    void funcSensorReadTask();
    static void sensorReadTask(void *pvParameters);
    void startSensorReadTask();
    void readBatteryResistorDivisor();
    void readBatteryBMS();
    void enviarComandoLeitura();
    bool validarChecksum(uint8_t *data, int len);
    void interpretarResposta(uint8_t *data);

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