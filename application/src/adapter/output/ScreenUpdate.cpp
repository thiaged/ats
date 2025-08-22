#include "ScreenUpdate.h"

ScreenUpdate::ScreenUpdate(
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
    hidden = true;

    updateSprite = new TFT_eSprite(&display);
    restartSprite = new TFT_eSprite(&display);
}

void ScreenUpdate::Render()
{
    Screen::Render();
    if ((millis() - slowScreenAnimUpdate) > 500)
    {
        slowScreenAnimUpdate = millis();
        drawUpdate();

        if (restartSystem) {
            if ((millis() - restartIniMillis) > restartDelayMiliseconds) {
                ESP.restart(); // Reinicia o ESP32
            }
        }
    }
}

void ScreenUpdate::SetFirmwareSize(unsigned long value)
{
    firmwareSize = value;
}

void ScreenUpdate::IncreaseProcessedSize(unsigned long value)
{
    firmwareSizeProcessed += value;
}

void ScreenUpdate::PrintProgress()
{
    Serial.printf("%u / %u\n", firmwareSizeProcessed, firmwareSize);
}

void ScreenUpdate::SetRestart(bool value)
{
    restartSystem = value;
    if (value) {
        restartIniMillis = millis();
    }
}

void ScreenUpdate::drawUpdate()
{
    updateSprite->createSprite(screenW, screenH);
    updateSprite->setTextColor(TFT_DARKGREY);
    updateSprite->setTextSize(1);
    updateSprite->setTextPadding(0);
    updateSprite->fillSprite(BG_WAVE);

    updateSprite->drawRoundRect(4, 23, 280, 30, 3, COLOR_HOUSE_TOP); // progress border

    int level = map(firmwareSizeProcessed, 0, firmwareSize, 0, 276);
    updateSprite->fillRoundRect(6, 25, level, 26, 3, COLOR_HOUSE_BASE);

    int progress = (firmwareSizeProcessed * 100) / firmwareSize;
    if (progress > 50) {
        updateSprite->setTextColor(TFT_YELLOW);
    }

    String percent = String(progress) + "%";
    updateSprite->drawString(percent.c_str(), 130, 28, 4);

    if (restartSystem) {
        updateSprite->setTextColor(TFT_DARKGREY);
        int endTime = (restartDelayMiliseconds - (millis() - restartIniMillis));
        if (endTime < 0) endTime = 0;
        endTime = endTime / 1000;
        String msg = "Reiniciando em " + String(endTime) + " ...";
        updateSprite->drawString(msg, 2, 62, 4);
    }

    drawManager.RequestDraw(screenX, screenY, screenW, screenH, updateSprite, nullptr, nullptr);
}
