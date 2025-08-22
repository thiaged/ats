#include "EnergySource.h"
#include "string.h"
#include "cmath"

EnergySource::EnergySource(
    Preferences &pConfigPreferences,
    DynamicAnalogBuffer &pBuffer,
    SourceType pType,
    int pSensorPin
)
    : configPreferences(pConfigPreferences), buffer(pBuffer)
{

    type = pType;
    sensorPin = pSensorPin;

    configPreferences.begin("config", true);
    voltageCalibration = configPreferences.getFloat("ac-calib", 0.202);
    configPreferences.end();
}

void EnergySource::SetStatus(EnergyStatus newStatus)
{
    if (newStatus == EnergyStatus::ACTIVE) {
        if (lastRecovery == 0) {
            lastRecovery = esp_timer_get_time();
            return;
        }

        if ((esp_timer_get_time() - lastRecovery) < delayRecovery) {
            return;
        }

        status = newStatus;
    } else {
        status = newStatus;
    }
    lastRecovery = 0;
}

float EnergySource::GetVoltageCalibration()
{
    return voltageCalibration;
}

EnergyStatus EnergySource::GetStatus()
{
    return status;
}

String EnergySource::GetStatusPrintable()
{
    switch (status) {
        case EnergyStatus::ACTIVE : return "active";
        case EnergyStatus::INACTIVE : return "inactive";
        default: return "invalid";
    }
}

float EnergySource::GetTension()
{
    return tension;
}

int EnergySource::GetSensorPin()
{
    return sensorPin;
}

SourceType EnergySource::GetType()
{
    return type;
}

String EnergySource::GetTypePrintable()
{
    switch (type) {
        case SourceType::SOLAR_SOURCE_TYPE : return "solar";
        case SourceType::UTILITY_SOURCE_TYPE : return "utility";
        default: return "invalid";
    }
}

uint64_t EnergySource::GetLastRecovery()
{
    return lastRecovery;
}

uint64_t EnergySource::GetDelayRecovery()
{
    return delayRecovery;
}

WaveDirection EnergySource::GetWaveDirection()
{
    return waveDirection;
}


void EnergySource::AddBufferValue(uint16_t value)
{
    buffer.AddReadingAc(value);
    
    // Improved filtering algorithm for more stable AC tension
    float newTension = buffer.GetRms() * voltageCalibration;
    
    // Use exponential moving average with adaptive alpha
    float alpha = 0.15f; // Increased from 0.02 for faster response
    
    // Apply adaptive filtering based on voltage stability
    if (tension > 0) {
        float voltageChange = abs(newTension - tension) / tension;
        
        // If voltage change is small, use more aggressive filtering
        if (voltageChange < 0.05f) { // Less than 5% change
            alpha = 0.05f; // Very stable - use more filtering
        } else if (voltageChange < 0.15f) { // Less than 15% change
            alpha = 0.15f; // Moderate change - balanced filtering
        } else {
            alpha = 0.30f; // Large change - faster response
        }
    }
    
    // Apply the filter
    tension = (tension == 0) ? newTension : (alpha * newTension + (1.0f - alpha) * tension);

    if ((tension < 90.0 || tension > 150) && status == EnergyStatus::ACTIVE) {
        SetStatus(EnergyStatus::INACTIVE);
    }
}

void EnergySource::ProcessSensorValue(unsigned int sensorRead)
{
    sensorValue = sensorRead;

    if (sensorValue > topAdcValue)
    {
        topAdcValue = sensorValue;
    }

    if (sensorValue < bottomAdcValue)
    {
        bottomAdcValue = sensorValue;
    }

    if (sensorValue > lastSensorValue)
    {
        countRisingSignal++;
    }else {
        countClimbingSignal++;
    }

    if (countRisingSignal > 4) {
        waveDirection = UP;
        countRisingSignal = 0;
        countClimbingSignal = 0;
    }

    if (countClimbingSignal > 4) {
        waveDirection = DOWN;
        countClimbingSignal = 0;
        countRisingSignal = 0;
    }

    if (abs(int(sensorValue - 1960)) < 200)
    {
        if (unstableSamples < 100)
        {
            unstableSamples++;
        }
    }
    else
    {
        unstableSamples = 0;
    }

    lastSensorValue = sensorValue;

    SetStatus((unstableSamples < debounceTime) ? ACTIVE : INACTIVE);

    AddBufferValue(sensorValue);
}

bool EnergySource::IsOnline()
{
    return status == ACTIVE;
}

unsigned int EnergySource::GetSensorValue()
{
    return sensorValue;
}

std::deque<unsigned long long> EnergySource::GetBufferDraw()
{
    return buffer.GetBufferDraw();
}

unsigned int EnergySource::GetTopAdcValue()
{
    return topAdcValue;
}

unsigned int EnergySource::GetBottomAdcValue()
{
    return bottomAdcValue;
}

void EnergySource::AutoCalibrateDcOffset()
{
    if (!isCalibrated) {
        if (calibrationStartTime == 0) {
            calibrationStartTime = millis();
            // Reset min/max values for calibration
            topAdcValue = 0;
            bottomAdcValue = 4096;
        }
        
        // Collect samples for 5 seconds
        if ((millis() - calibrationStartTime) < CALIBRATION_DURATION) {
            // Update min/max during calibration
            if (sensorValue > topAdcValue) {
                topAdcValue = sensorValue;
            }
            if (sensorValue < bottomAdcValue) {
                bottomAdcValue = sensorValue;
            }
        } else {
            // Calibration complete - calculate optimal DC offset
            uint32_t calculatedOffset = (topAdcValue + bottomAdcValue) / 2;
            
            // Update the DC offset in the buffer
            buffer.SetDcOffset(calculatedOffset);
            
            // Reset calibration state
            isCalibrated = true;
            calibrationStartTime = 0;
            
            Serial.printf("Auto-calibration complete. DC Offset: %d (Min: %d, Max: %d)\n", 
                         calculatedOffset, bottomAdcValue, topAdcValue);
        }
    }
}

bool EnergySource::IsCalibrated() const
{
    return isCalibrated;
}

void EnergySource::TriggerRecalibration()
{
    isCalibrated = false;
    calibrationStartTime = 0;
    topAdcValue = 0;
    bottomAdcValue = 4096;
    Serial.println("Recalibration triggered for " + GetTypePrintable() + " source");
}
