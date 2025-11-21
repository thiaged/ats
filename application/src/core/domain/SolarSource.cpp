#include "SolarSource.h"

SolarSource::SolarSource(
    Preferences &pConfigPreferences,
    DynamicAnalogBuffer &pBuffer,
    SourceType pType,
    int pSensorPin
)
    : EnergySource(pConfigPreferences, pBuffer, pType, pSensorPin)
{
    configPreferences.begin("config", true);
    buffer.SetDcOffset(configPreferences.getInt("sol-offset", 1940)); //maximo 15 char on name
    voltageCalibration = configPreferences.getFloat("ac-calib-sol", 0.195);
    configPreferences.end();
}

void SolarSource::SetVoltageOffset(uint32_t value)
{
    bool changed = value != buffer.GetDcOffset();
    buffer.SetDcOffset(value);
    if (changed)
    {

        configPreferences.begin("config", false);
        configPreferences.putInt("sol-offset", value); //maximo 15 char on name
        configPreferences.end();
    }
}

void SolarSource::SetVoltageCalibration(float value)
{
    bool changed = value != voltageCalibration;
    voltageCalibration = value;
    if (changed)
    {
        configPreferences.begin("config", false);
        configPreferences.putFloat("ac-calib-sol", value); //maximo 15 char on name
        configPreferences.end();
    }
}
