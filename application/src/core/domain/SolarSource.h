#pragma once
#include "EnergySource.h"

class SolarSource : public EnergySource {
    public:
        SolarSource(
            Preferences &pConfigPreferences,
            DynamicAnalogBuffer &pBuffer,
            SourceType pType,
            int pSensorPin
        );

        void SetVoltageOffset(uint32_t value) override;
        void SetVoltageCalibration(float value) override;
};