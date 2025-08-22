#include "ScreenClock.h"

ScreenClock::ScreenClock(
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
    clockSprite = new TFT_eSprite(&display);

    temp_sensor_config_t temp_sensor = {
        .dac_offset = TSENS_DAC_L2,
        .clk_div = 6,
    };
    temp_sensor_set_config(temp_sensor);
    temp_sensor_start();
}

void ScreenClock::Render()
{
    Screen::Render();
    if ((millis() - slowScreenAnimClock) > 500)
    {
        slowScreenAnimClock = millis();
        drawClock();
    }

}

void ScreenClock::drawClock()
{
    time_t now = time(nullptr);
    struct tm *timeinfo = localtime(&now);

    clockSprite->createSprite(screenW, screenH);
    clockSprite->setTextColor(TFT_YELLOW);
    clockSprite->setTextSize(1);
    clockSprite->setTextPadding(0);
    clockSprite->fillSprite(BG_WAVE);
    char hora[10];
    char data[15];
    sprintf(hora, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    sprintf(data, "%02d/%02d/%04d", timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900);
    clockSprite->drawString(hora, 0, 10, 7);
    clockSprite->drawString(data, 0, 80, 4);

    float tsens_out;
    temp_sensor_read_celsius(&tsens_out);
    String temp = "CPU: " + String(tsens_out) + " graus";
    clockSprite->drawString(temp, 0, 110, 4);

    drawManager.RequestDraw(screenX, screenY, screenW, screenH, clockSprite, nullptr, nullptr);
}