#pragma once
#include <adapter/output/Screen.h>

class ScreenUpdate : public Screen
{
private:
    unsigned long firmwareSize;
    unsigned long firmwareSizeProcessed = 0;
    TFT_eSprite* updateSprite;
    TFT_eSprite* restartSprite;

    unsigned long slowScreenAnimUpdate = 0;
    bool restartSystem = false;
    u16_t restartDelayMiliseconds = 6000;
    unsigned long restartIniMillis = 0;

    void drawUpdate();

public:
    ScreenUpdate(
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
    virtual ~ScreenUpdate() = default;

    void Render() override;

    void SetFirmwareSize(unsigned long value);
    void IncreaseProcessedSize(unsigned long value);
    void PrintProgress();
    void SetRestart(bool value);
};