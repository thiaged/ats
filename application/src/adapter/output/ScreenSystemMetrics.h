#pragma once
#include <adapter/output/Screen.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <freertos/task.h>

class ScreenSystemMetrics : public Screen
{
private:
    TFT_eSprite* metricsSprite;
    unsigned long slowScreenAnimMetrics = 0;

    void drawSystemMetrics();
    void drawMemoryMetrics();
    void drawCPUMetrics();

public:
    ScreenSystemMetrics(
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
    virtual ~ScreenSystemMetrics() = default;
    void Render() override;
};

