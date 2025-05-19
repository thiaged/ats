#pragma once
#include "EnergySource.h"

class UtilitySource : public EnergySource {
    public:
        UtilitySource(
            Preferences &pConfigPreferences,
            DynamicAnalogBuffer &pBuffer,
            SourceType pType,
            int pSensorPin
        );
        void SetVoltageOffset(uint32_t value) override;
        void SetVoltageCalibration(float value) override;
};