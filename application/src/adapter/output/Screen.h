#pragma once
#include <TFT_eSPI.h>
#include <core/domain/BatterySource.h>
#include <adapter/output/DrawManager.h>
#include <Preferences.h>

#define BORDER_WAVE 0xC480
#define BG_WAVE 0xD69A
#define ADC_SCALE 3300 // 4096 = 220
#define VREF 3.3
#define ZEROPOINT 1950
#define FREQUENCY 60
#define COLOR_WAVE_UTILITY TFT_RED
#define COLOR_WAVE_SOLAR TFT_BLUE

#define COLOR_HOUSE_BASE 0x34D4
#define COLOR_HOUSE_TOP 0x34D8

#define BG_SOURCE 0x1AAF
#define BG_SOURCE_INACTIVE 0x8C51
#define BG_SYSTEM 0xBDF7
#define COLOR_BATTERY_VOLTAGE 0xFF20
#define COLOR_BATTERY_SYMBOL_BORDER 0xEC31
#define COLOR_BATTERY_SYMBOL_ENERGY 0x4479
#define COLOR_BATTERY_SYMBOL_ENERGY_LOW 0xD4D3

class Screen {
private:

protected:
    BatterySource &battery;
    DrawManager &drawManager;
    TFT_eSPI &display;
    LilyGo_Class &amoled;
    EnergySource &utilitySource;
    EnergySource &solarSource;
    EnergySource **activeSource;
    EnergySource **definedSource;
    EnergySource **userDefinedSource;
    bool *userSourceLocked;
    Preferences &configPreferences;

    TFT_eSprite* utilitySprite;    // Create a sprite for the transmission line tower
    TFT_eSprite* utilityLineSprite;    // Create a sprite for the transmission line tower
    TFT_eSprite* utilityTensionSprite;    // Create a sprite for the transmission line tower

    TFT_eSprite* solarSprite;      // Create a sprite for the solar panel array and sun
    TFT_eSprite* solarLineSprite;      // Create a sprite for the solar panel array and sun
    TFT_eSprite* solarTensionSprite;      // Create a sprite for the solar panel array and sun

    TFT_eSprite *batteryVoltageSprite;

    TFT_eSprite *sysBatterySprite;

    bool hidden = false;

    const int screenW = 350;
    const int screenH = 200;
    const int screenX = 180;
    const int screenY = 20;

    const int posLineX = 86;
    int posEnergyUtility = 0;
    const int posUtilityLineY = 192;

    unsigned long slowScreenAnim = 0;

    int posEnergySolar = 0;
    const int posSolarLineY = 44;

    void drawUtilitySource();
    void drawSolarSource();
    void drawBatteryVoltage();
    void drawSysBattery();

public:
    Screen(
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

    bool redrawHouse = true;

    virtual void Render();
    bool isHidden();

};
