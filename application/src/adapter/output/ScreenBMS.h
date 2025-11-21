#pragma once
#include <adapter/output/Screen.h>

class ScreenBMS : public Screen
{
private:
    TFT_eSprite* bmsSprite;
    unsigned long slowScreenAnimBms = 0;
    unsigned long cellDataUpdateTime = 0;

    // Cell voltage display variables
    int cellDisplayStartIndex = 0;
    const int maxCellsPerScreen = 8;

    void drawBMSData();
    void drawCellVoltages();
    void drawBMSHeader();
    void drawBMSStats();

public:
    ScreenBMS(
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
    virtual ~ScreenBMS() = default;
    void Render() override;
};