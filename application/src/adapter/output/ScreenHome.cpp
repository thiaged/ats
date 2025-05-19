#include "ScreenHome.h"

ScreenHome::ScreenHome(
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
)
    : Screen(pUtilitySource, pSolarSource, pActiveSource, pDefinedSource, pUserDefinedSource, pBattery, pDrawManager, pDisplay,
        pAmoled, pUserSourceLocked, pConfigPreferences)
{
    batterySprite = new TFT_eSprite(&display);
    homeSprite = new TFT_eSprite(&display);
    homeAnimatedSprite = new TFT_eSprite(&display);
}

void ScreenHome::drawBattery()
{
    uint16_t colorEnergy = COLOR_BATTERY_SYMBOL_ENERGY;
    if (battery.IsLowBattery())
    {
        colorEnergy = COLOR_BATTERY_SYMBOL_ENERGY_LOW;
    }

    // Set up the sprite dimensions (80x80 size)
    batterySprite->createSprite(84, 95);  // Width and height for the sprite
    batterySprite->fillSprite(BG_SYSTEM); // Fill the sprite background

    // bat body
    batterySprite->fillRoundRect(8, 0, 14, 5, 3, COLOR_BATTERY_SYMBOL_BORDER);  // positive pole
    batterySprite->drawRoundRect(0, 3, 30, 92, 3, COLOR_BATTERY_SYMBOL_BORDER); // body
    batterySprite->drawRect(1, 4, 28, 90, COLOR_BATTERY_SYMBOL_BORDER);

    // bat energy
    int level = map(battery.GetBatteryVoltage() * 100, battery.GetBatteryVoltageMin() * 100, battery.GetBatteryVoltageMax() * 100, 0, 88);
    int linePosMax = 88 - map(battery.GetBatteryConfigMax() * 100, battery.GetBatteryVoltageMin() * 100, battery.GetBatteryVoltageMax() * 100, 0, 88);
    int linePosMin = 88 - map(battery.GetBatteryConfigMin() * 100, battery.GetBatteryVoltageMin() * 100, battery.GetBatteryVoltageMax() * 100, 0, 88);
    batterySprite->fillRect(2, (5 + 88 - level), 26, level, colorEnergy);

    // configs min and max
    batterySprite->drawLine(12, 5 + linePosMax, 40, 5 + linePosMax, TFT_WHITE);
    batterySprite->drawString(String(battery.GetBatteryConfigMax()), 40, linePosMax, 2);

    batterySprite->drawLine(12, 5 + linePosMin, 40, 5 + linePosMin, TFT_WHITE);
    batterySprite->drawString(String(battery.GetBatteryConfigMin()), 40, linePosMin, 2);

    drawManager.RequestDraw(440, 74, 84, 95, batterySprite, nullptr, nullptr);
}

void ScreenHome::drawScreenHome()
{
    // Fatores de escala baseados no layout original (350x160)
    float scaleX = screenW / 350.0;
    float scaleY = screenH / 160.0;

    if (redrawHouse)
    {
        homeSprite->createSprite(screenW, screenH);
        homeSprite->fillSprite(BG_SYSTEM);

        // Draw left rect
        homeSprite->fillRect(0, 0, 60, screenH, COLOR_HOUSE_BASE);

        // Draw house base (original: x=154, y=63, w=80, h=65)
        homeSprite->fillRect((int)(154 * scaleX), (int)(63 * scaleY), (int)(80 * scaleX), (int)(65 * scaleY), COLOR_HOUSE_BASE);

        // Draw house top (triangle) (original: 154,63, 194,23, 234,63)
        homeSprite->fillTriangle((int)(154 * scaleX), (int)(63 * scaleY),
                                (int)(194 * scaleX), (int)(23 * scaleY),
                                (int)(234 * scaleX), (int)(63 * scaleY),
                                COLOR_HOUSE_BASE);

        // Draw door (original: x=181, y=94, w=25, h=34)
        homeSprite->fillRect((int)(181 * scaleX), (int)(94 * scaleY), (int)(25 * scaleX), (int)(34 * scaleY), BG_SYSTEM);

        // Draw roof lines (original coordenadas ajustadas)
        homeSprite->drawWideLine((int)(143 * scaleX), (int)(69 * scaleY),
                                (int)(162 * scaleX), (int)(50 * scaleY),
                                4, COLOR_HOUSE_TOP);

        homeSprite->drawWideLine((int)(162 * scaleX), (int)(50 * scaleY),
                                (int)(162 * scaleX), (int)(26 * scaleY),
                                4, COLOR_HOUSE_TOP);

        homeSprite->drawWideLine((int)(162 * scaleX), (int)(26 * scaleY),
                                (int)(172 * scaleX), (int)(26 * scaleY),
                                4, COLOR_HOUSE_TOP);

        homeSprite->drawWideLine((int)(172 * scaleX), (int)(26 * scaleY),
                                (int)(172 * scaleX), (int)(40 * scaleY),
                                4, COLOR_HOUSE_TOP);

        homeSprite->drawWideLine((int)(172 * scaleX), (int)(40 * scaleY),
                                (int)(194 * scaleX), (int)(18 * scaleY),
                                4, COLOR_HOUSE_TOP);

        homeSprite->drawWideLine((int)(194 * scaleX), (int)(18 * scaleY),
                                (int)(245 * scaleX), (int)(69 * scaleY),
                                4, COLOR_HOUSE_TOP);


        drawManager.RequestDraw(screenX, screenY, screenW, screenH, homeSprite, nullptr, nullptr);

        redrawHouse = false;
    }
    homeAnimatedSprite->createSprite(86, 14);
    homeAnimatedSprite->fillSprite(BG_SYSTEM);
    for (int i = 0, pos = 0; i < 5; i++, pos = pos + 18)
    {
        if (i == animatedHomePos && (solarSource.IsOnline() || utilitySource.IsOnline()))
        {
            homeAnimatedSprite->fillRect(pos, 0, 14, 14, TFT_WHITE);
        }
        else
        {
            homeAnimatedSprite->fillRect(pos, 0, 14, 14, COLOR_HOUSE_BASE);
        }
    }
    drawManager.RequestDraw(screenX + 60 + 4, screenY + (int)(80 * scaleY), 86, 14, homeAnimatedSprite, nullptr, nullptr);
    animatedHomePos++;
    if (animatedHomePos >= 5)
    {
        animatedHomePos = 0;
    }
}

void ScreenHome::Render()
{
    Screen::Render();
    if (millis() - slowScreenAnimHome > 200)
    {
        slowScreenAnimHome = millis();
        drawBattery();
        drawScreenHome();
    }
}
