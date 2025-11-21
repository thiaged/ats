#include "UtilitySource.h"

UtilitySource::UtilitySource(
    Preferences &pConfigPreferences,
    DynamicAnalogBuffer &pBuffer,
    SourceType pType,
    int pSensorPin
)
    : EnergySource(pConfigPreferences, pBuffer, pType, pSensorPin)
{
    configPreferences.begin("config", true);
    buffer.SetDcOffset(configPreferences.getInt("util-offset", 1940)); //maximo 15 char on name
    voltageCalibration = configPreferences.getFloat("ac-calib-uti", 0.195);
    configPreferences.end();
}

void UtilitySource::SetVoltageOffset(uint32_t value)
{
    bool changed = value != buffer.GetDcOffset();
    buffer.SetDcOffset(value);
    if (changed)
    {
        configPreferences.begin("config", false);
        configPreferences.putInt("util-offset", value); //maximo 15 char on name
        configPreferences.end();
    }
}

void UtilitySource::SetVoltageCalibration(float value)
{
    bool changed = value != voltageCalibration;
    voltageCalibration = value;
    if (changed)
    {
        configPreferences.begin("config", false);
        configPreferences.putFloat("ac-calib-uti", value); //maximo 15 char on name
        configPreferences.end();
    }
}
