#pragma once
#include <Arduino.h>
#include <esp_timer.h>
#include "DynamicAnalogBuffer.h"
#include <Preferences.h>

enum WaveDirection {
    UP = 1,
    DOWN = 0
};

enum EnergyStatus {
    ACTIVE = 1,
    INACTIVE = 0
};

enum SourceType {
    UTILITY_SOURCE_TYPE = 1,
    SOLAR_SOURCE_TYPE = 2
};

class EnergySource {
protected:
    Preferences &configPreferences;
    EnergyStatus status = EnergyStatus::INACTIVE;
    SourceType type;
    int sensorPin;
    float tension = 0.0;
    uint64_t lastRecovery = 0;
    uint64_t delayRecovery = 10000000; //atrasar recuperacao da rede, valor em microsegundos. 1.000.000 = 1s
    unsigned int sensorValue = 0;
    unsigned int lastSensorValue = 0;
    float voltageCalibration = 0.142;
    float alphaFilter = 0.1;
    unsigned int unstableSamples = 10;
    int countRisingSignal = 0;
    int countClimbingSignal = 0;
    DynamicAnalogBuffer &buffer;

    WaveDirection waveDirection = UP;
    unsigned const int debounceTime = 15;

    unsigned int topAdcValue = 0;
    unsigned int bottomAdcValue = 5000;

public:
    EnergySource(
        Preferences &pConfigPreferences,
        DynamicAnalogBuffer &pBuffer,
        SourceType pType,
        int pSensorPin
    );

    unsigned int maxVal = 0;
    unsigned int minVal = 4096;

    virtual ~EnergySource() = default;
    void SetStatus(EnergyStatus status);
    virtual void SetVoltageCalibration(float value) = 0;
    float GetVoltageCalibration();
    EnergyStatus GetStatus();
    String GetStatusPrintable();
    float GetTension();
    int GetSensorPin();
    SourceType GetType();
    String GetTypePrintable();
    uint64_t GetLastRecovery();
    uint64_t GetDelayRecovery();
    WaveDirection GetWaveDirection();
    void AddBufferValue(uint16_t value);
    void ProcessSensorValue(unsigned int sensorRead);
    bool IsOnline();
    unsigned int GetSensorValue();
    std::deque<unsigned long long> GetBufferDraw();
    virtual void SetVoltageOffset(uint32_t value) = 0;
    unsigned int GetTopAdcValue();
    unsigned int GetBottomAdcValue();
    
    // Auto-calibration methods for better stability
    void AutoCalibrateDcOffset();
    bool IsCalibrated() const;
    void TriggerRecalibration();
    
private:
    bool isCalibrated = false;
    unsigned long calibrationStartTime = 0;
    static const unsigned long CALIBRATION_DURATION = 5000; // 5 seconds for calibration
};
