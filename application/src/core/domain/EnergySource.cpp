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
    tension = (tension < 10) ? (buffer.GetRms() * voltageCalibration) : 0.05 * (buffer.GetRms() * voltageCalibration) + 0.95 * tension;

    if ((tension < 90.0 || tension > 140) && status == EnergyStatus::ACTIVE) {
        SetStatus(EnergyStatus::INACTIVE);
    }
}

void EnergySource::ProcessSensorValue(unsigned int sensorRead)
{
    // sensorValue = (sensorValue < 10) ? sensorRead : lround(alphaFilter * sensorRead + (1 - alphaFilter) * sensorValue);
    // sensorValue = lround(sensorRead / 10) * 10;
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
