#pragma once
#include <adapter/output/Screen.h>

class ScreenClock : public Screen
{
private:
    TFT_eSprite* clockSprite;

    unsigned long slowScreenAnimClock = 0;

    void drawClock();

public:
    ScreenClock(
        EnergySource &pUtilitySource,
        EnergySource &pSolarSource,
        EnergySource **pActiveSource,
        EnergySource **pDefinedSource,
        EnergySource **pUserDefinedSource,
        BatterySource &pBattery,
        DrawManager &pDrawManager,
        TFT_eSPI &pDisplay,
        LilyGo_Class &pAmoled,
        bool *pUserSourceLocked,
        Preferences &pConfigPreferences
    );
    virtual ~ScreenClock() = default;

    void Render() override;

};