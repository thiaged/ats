#pragma once
#include "Screen.h"

class ScreenHome : public Screen
{
private:
    TFT_eSprite *batterySprite;
    TFT_eSprite *homeSprite;
    TFT_eSprite *homeAnimatedSprite;

    // const int homeX = 180;
    // const int homeY = 40;

    int animatedHomePos = 0;

    unsigned long slowScreenAnimHome = 0;

    void drawBattery();
    void drawScreenHome();

public:
    ScreenHome(
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

    void Render() override;
};
