#include "Screen.h"

Screen::Screen(
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
    : utilitySource(pUtilitySource), solarSource(pSolarSource), battery(pBattery), drawManager(pDrawManager), display(pDisplay),
        configPreferences(pConfigPreferences), amoled(pAmoled)
{
    activeSource = pActiveSource;
    definedSource = pDefinedSource;
    userSourceLocked = pUserSourceLocked;
    userDefinedSource = pUserDefinedSource;
    utilitySprite = new TFT_eSprite(&display);
    utilityLineSprite = new TFT_eSprite(&display);
    utilityTensionSprite = new TFT_eSprite(&display);

    solarSprite = new TFT_eSprite(&display);
    solarLineSprite = new TFT_eSprite(&display);
    solarTensionSprite = new TFT_eSprite(&display);

    batteryVoltageSprite = new TFT_eSprite(&display);
    sysBatterySprite = new TFT_eSprite(&display);

    amoled.enableBattDetection();
    amoled.enableBattVoltageMeasure();
}

void Screen::Render()
{
    if ((millis() - slowScreenAnim) > 200)
    {
        slowScreenAnim = millis();
        drawUtilitySource();
        drawSolarSource();
        drawBatteryVoltage();
        drawSysBattery();
    }
}

void Screen::drawUtilitySource()
{
    // Set up the sprite dimensions (80x80 size)
    utilitySprite->createSprite(80, 80); // Width and height for the tower sprite
    utilityLineSprite->createSprite(87, 10); // Width and height for the line sprite
    utilityLineSprite->fillSprite(BG_SYSTEM);

    utilityTensionSprite->createSprite(90, 30); // Width and height for the line sprite
    utilityTensionSprite->fillSprite(BG_SYSTEM);
    utilityTensionSprite->setTextColor(COLOR_WAVE_UTILITY);
    char tensionStr[10];
    // Converter o valor para uma string formatada com 2 casas decimais
    dtostrf(utilitySource.GetTension(), 7, 2, tensionStr); // 5 é o comprimento total mínimo da string (incluindo o ponto e as casas decimais)
    utilityTensionSprite->drawString(String(tensionStr), 0, 5, 4);

    if (utilitySource.IsOnline())
    {
        utilitySprite->fillSprite(BG_SOURCE); // Fill the sprite background
    }
    else
    {
        utilitySprite->fillSprite(BG_SOURCE_INACTIVE); // Fill the sprite background
    }

    // Draw the transmission tower based on an 80x80 size
    utilitySprite->drawLine(28, 70, 35, 41, TFT_WHITE);  // Left tower leg
    utilitySprite->drawLine(52, 70, 45, 41, TFT_WHITE);  // Right tower leg
    utilitySprite->drawLine(16, 70, 64, 70, TFT_WHITE); // Base floor
    utilitySprite->drawLine(40, 70, 40, 55, TFT_WHITE); // Base center
    utilitySprite->drawLine(40, 68, 33, 51, TFT_WHITE); // Base left
    utilitySprite->drawLine(40, 68, 47, 51, TFT_WHITE); // Base right

    utilitySprite->drawLine(31, 60, 47, 51, TFT_WHITE); // leg diagonal left
    utilitySprite->drawLine(33, 51, 45, 43, TFT_WHITE); // leg diagonal left
    utilitySprite->drawLine(33, 51, 49, 60, TFT_WHITE); // leg diagonal right
    utilitySprite->drawLine(35, 43, 47, 51, TFT_WHITE); // leg diagonal right

    utilitySprite->drawLine(35, 41, 35, 9, TFT_WHITE); // body left
    utilitySprite->drawLine(45, 41, 45, 9, TFT_WHITE); // body right

    utilitySprite->drawLine(18, 43, 34, 43, TFT_WHITE); // line arm left
    utilitySprite->drawLine(18, 33, 35, 33, TFT_WHITE); // line arm left
    utilitySprite->drawLine(18, 23, 35, 23, TFT_WHITE); // line arm left
    utilitySprite->drawLine(18, 13, 35, 13, TFT_WHITE); // line arm left

    utilitySprite->drawLine(46, 43, 62, 43, TFT_WHITE); // line arm left
    utilitySprite->drawLine(45, 33, 62, 33, TFT_WHITE); // line arm left
    utilitySprite->drawLine(45, 23, 62, 23, TFT_WHITE); // line arm left
    utilitySprite->drawLine(45, 13, 62, 13, TFT_WHITE); // line arm left

    utilitySprite->drawLine(21, 43, 21, 47, TFT_WHITE); // line transmission left
    utilitySprite->drawLine(21, 33, 21, 37, TFT_WHITE); // line transmission left
    utilitySprite->drawLine(21, 23, 21, 27, TFT_WHITE); // line transmission left
    utilitySprite->drawLine(21, 13, 21, 17, TFT_WHITE); // line transmission left

    utilitySprite->drawLine(59, 43, 59, 47, TFT_WHITE); // line transmission right
    utilitySprite->drawLine(59, 33, 59, 37, TFT_WHITE); // line transmission right
    utilitySprite->drawLine(59, 23, 59, 27, TFT_WHITE); // line transmission right
    utilitySprite->drawLine(59, 13, 59, 17, TFT_WHITE); // line transmission right

    utilitySprite->drawLine(35, 43, 45, 37, TFT_WHITE); // diagonal body left
    utilitySprite->drawLine(21, 43, 45, 33, TFT_WHITE); // diagonal body left
    utilitySprite->drawLine(35, 33, 45, 27, TFT_WHITE); // diagonal body left
    utilitySprite->drawLine(21, 33, 45, 23, TFT_WHITE); // diagonal body left
    utilitySprite->drawLine(35, 23, 45, 17, TFT_WHITE); // diagonal body left
    utilitySprite->drawLine(21, 23, 45, 13, TFT_WHITE); // diagonal body left
    utilitySprite->drawLine(35, 13, 45, 9, TFT_WHITE); // diagonal body left
    utilitySprite->drawLine(21, 13, 45, 6, TFT_WHITE); // diagonal body left

    utilitySprite->drawLine(35, 37, 45, 43, TFT_WHITE); // diagonal body right
    utilitySprite->drawLine(35, 33, 59, 43, TFT_WHITE); // diagonal body right
    utilitySprite->drawLine(35, 27, 45, 33, TFT_WHITE); // diagonal body right
    utilitySprite->drawLine(35, 23, 59, 33, TFT_WHITE); // diagonal body right
    utilitySprite->drawLine(35, 17, 45, 23, TFT_WHITE); // diagonal body right
    utilitySprite->drawLine(35, 13, 59, 23, TFT_WHITE); // diagonal body right
    utilitySprite->drawLine(35, 9, 45, 13, TFT_WHITE); // diagonal body right
    utilitySprite->drawLine(40, 6, 59, 13, TFT_WHITE); // diagonal body right

    int lineLeng = 86;
    int dotSpace = 10;
    int dotSize = 5;
    if ((*activeSource)->GetType() == UTILITY_SOURCE_TYPE && (*activeSource)->IsOnline())
    {
        for (int i = 0; i < lineLeng; i = i + dotSpace)
        {
            int x0 = i;
            i = i + dotSize;
            utilityLineSprite->fillRect(x0, 3, dotSize, 4, BG_SOURCE);
        }
        utilityLineSprite->fillCircle(posEnergyUtility, 5, 4, TFT_WHITE);
        posEnergyUtility += 5;
        if (posEnergyUtility > lineLeng)
        {
            posEnergyUtility = 0;
        }
    }
    else
    {
        if (utilitySource.IsOnline())
        {
            for (int i = 0; i < lineLeng; i = i + dotSpace)
            {
                int x0 = i;
                i = i + dotSize;
                utilityLineSprite->fillRect(x0, 3, dotSize, 4, BG_SOURCE_INACTIVE);
            }
            posEnergyUtility = posLineX;
        }
    }

    if(! utilitySource.IsOnline()) {
        utilitySprite->drawLine(0, 0, 80, 80, TFT_RED);
        utilitySprite->drawLine(0, 80, 80, 0, TFT_RED);

        if (utilitySource.GetLastRecovery() > 0){
            utilitySprite->fillRoundRect(0, 64, 80, 10, 5, TFT_LIGHTGREY);
            utilitySprite->fillRoundRect(2, 66, map(esp_timer_get_time() - utilitySource.GetLastRecovery(), 0, utilitySource.GetDelayRecovery(), 0, 76), 8, 4, TFT_CYAN);
        }
    }

    if (*userDefinedSource != nullptr) {
        if ((*userDefinedSource)->GetType() == SourceType::UTILITY_SOURCE_TYPE) {
            utilitySprite->fillCircle(66, 35, 8, TFT_WHITE);
            if (*userSourceLocked) {
                utilitySprite->drawCircle(66, 35, 8, TFT_RED);
                utilitySprite->drawCircle(66, 35, 9, TFT_RED);
            }
        }
    }

    if ((*definedSource)->GetType() == SourceType::UTILITY_SOURCE_TYPE) {
        utilitySprite->fillCircle(66, 35, 6, TFT_CYAN);
    }

    drawManager.RequestDraw(4, 156, 80, 80, utilitySprite, nullptr, nullptr);
    drawManager.RequestDraw(posLineX, posUtilityLineY, 87, 10, utilityLineSprite, nullptr, nullptr);
    drawManager.RequestDraw(posLineX, drawManager.GetScreenHeight() - 30, 90, 30, utilityTensionSprite, nullptr, nullptr);

}

void Screen::drawSolarSource()
{
    uint32_t panelLineColor = 0xBDF7;
    uint32_t sunColor = 0xFCC0;
    uint32_t sunWaveColor = 0xFE4E;
    // Set up the sprite dimensions (80x80 size)
    solarSprite->createSprite(80, 80); // Width and height for the sprite
    solarLineSprite->createSprite(87, 10); // Width and height for the line sprite
    solarLineSprite->fillSprite(BG_SYSTEM);

    solarTensionSprite->createSprite(90, 40); // Width and height for the line sprite
    solarTensionSprite->fillSprite(BG_SYSTEM);
    solarTensionSprite->setTextColor(COLOR_WAVE_SOLAR);
    char tensionStr[10];
    // Converter o valor para uma string formatada com 2 casas decimais
    dtostrf(solarSource.GetTension(), 7, 2, tensionStr); // 5 é o comprimento total mínimo da string (incluindo o ponto e as casas decimais)
    solarTensionSprite->drawString(String(tensionStr), 0, 5, 4);

    if (solarSource.IsOnline())
    {
        solarSprite->fillSprite(BG_SOURCE); // Fill the sprite background
    }
    else
    {
        solarSprite->fillSprite(BG_SOURCE_INACTIVE); // Fill the sprite background
        sunColor = panelLineColor;
        sunWaveColor = panelLineColor;
    }

    // Desenhar o sol (círculo simples com raios)
    solarSprite->fillCircle(19, 19, 8, sunColor); // Corpo do sol

    for (int i = 0; i < 8; i++) {
        // Calcular ângulo em radianos (passos de 45 graus)
        float angle = i * PI / 4; // 45 graus = PI/4 radianos

        // Calcular a posição do centro do triângulo a 12px do centro do círculo
        int centerX = 20 + 12 * cos(angle);
        int centerY = 20 + 12 * sin(angle);

        // Definir o tamanho do triângulo
        float size = 4.62; // Tamanho de lado para uma altura de ~4 pixels

        // Calcular os vértices do triângulo com base no centro e no ângulo de rotação
        int x1 = centerX + size * cos(angle);             // Vértice superior
        int y1 = centerY + size * sin(angle);
        int x2 = centerX + size * cos(angle + 2 * PI / 3); // Vértice inferior esquerdo
        int y2 = centerY + size * sin(angle + 2 * PI / 3);
        int x3 = centerX + size * cos(angle + 4 * PI / 3); // Vértice inferior direito
        int y3 = centerY + size * sin(angle + 4 * PI / 3);

        // Desenhar o triângulo em torno do círculo
        solarSprite->fillTriangle(x1, y1, x2, y2, x3, y3, sunWaveColor);
    }

    // Draw solar panel
    solarSprite->drawLine(20, 60, 46, 16, panelLineColor);
    solarSprite->drawLine(25, 59, 48, 15, panelLineColor);
    solarSprite->drawLine(30, 59, 50, 14, panelLineColor);
    solarSprite->drawLine(35, 59, 52, 13, panelLineColor);
    solarSprite->drawLine(40, 59, 55, 11, panelLineColor);
    solarSprite->drawLine(46, 58, 58, 9, panelLineColor);
    solarSprite->drawLine(52, 58, 61, 8, panelLineColor);
    solarSprite->drawLine(59, 58, 64, 6, panelLineColor);
    solarSprite->drawLine(66, 58, 67, 4, panelLineColor);

    solarSprite->drawLine(20, 60, 66, 58, panelLineColor);
    solarSprite->drawLine(25, 52, 66, 46, panelLineColor);
    solarSprite->drawLine(30, 44, 66, 36, panelLineColor);
    solarSprite->drawLine(34, 37, 66, 27, panelLineColor);
    solarSprite->drawLine(38, 30, 66, 19, panelLineColor);
    solarSprite->drawLine(41, 25, 66, 14, panelLineColor);
    solarSprite->drawLine(43, 21, 67, 9, panelLineColor);
    solarSprite->drawLine(46, 16, 67, 4, panelLineColor);

    int lineLeng = 86;
    int dotSpace = 10;
    int dotSize = 5;
    if ((*activeSource)->GetType() == SOLAR_SOURCE_TYPE && (*activeSource)->IsOnline())
    {
        for (int i = 0; i < lineLeng; i = i + dotSpace)
        {
            int x0 = i;
            i = i + dotSize;
            solarLineSprite->fillRect(x0, 3, dotSize, 4, BG_SOURCE);
        }

        solarLineSprite->fillCircle(posEnergySolar, 5, 4, TFT_WHITE);
        posEnergySolar = posEnergySolar + 5;
        if (posEnergySolar > lineLeng)
        {
            posEnergySolar = 0;
        }
    }
    else
    {
        if (solarSource.IsOnline())
        {
            for (int i = 0; i < lineLeng; i = i + dotSpace)
            {
                int x0 = i;
                i = i + dotSize;
                solarLineSprite->fillRect(x0, 3, dotSize, 4, BG_SOURCE_INACTIVE);
            }
            posEnergySolar = posLineX;
        }
    }

    // draw red X lines
    if (! solarSource.IsOnline()) {
        solarSprite->drawLine(0, 0, 80, 80, TFT_RED);
        solarSprite->drawLine(0, 80, 80, 0, TFT_RED);

        if (solarSource.GetLastRecovery() > 0){
            solarSprite->fillRoundRect(0, 64, 80, 10, 5, TFT_LIGHTGREY);
            solarSprite->fillRoundRect(2, 66, map(esp_timer_get_time() - solarSource.GetLastRecovery(), 0, solarSource.GetDelayRecovery(), 0, 76), 8, 4, TFT_CYAN);

        }
    }

    if (*userDefinedSource != nullptr) {
        if ((*userDefinedSource)->GetType() == SourceType::SOLAR_SOURCE_TYPE) {
            solarSprite->fillCircle(66, 35, 8, TFT_WHITE);
            if (*userSourceLocked) {
                solarSprite->drawCircle(66, 35, 8, TFT_RED);
                solarSprite->drawCircle(66, 35, 9, TFT_RED);
            }
        }
    }

    if ((*definedSource)->GetType() == SourceType::SOLAR_SOURCE_TYPE) {
        solarSprite->fillCircle(66, 35, 6, TFT_CYAN);
    }

    drawManager.RequestDraw(4, 4, 80, 80, solarSprite, nullptr, nullptr);
    drawManager.RequestDraw(posLineX, posSolarLineY, 87, 10, solarLineSprite, nullptr, nullptr);
    drawManager.RequestDraw(posLineX, 4, 90, 40, solarTensionSprite, nullptr, nullptr);

}

void Screen::drawBatteryVoltage()
{
    batteryVoltageSprite->createSprite(140, 60);
    batteryVoltageSprite->fillSprite(BG_SYSTEM);
    char tensionStr[10];
    // Converter o valor para uma string formatada com 2 casas decimais
    dtostrf(battery.GetBatteryVoltage(), 5, 2, tensionStr);
    batteryVoltageSprite->setTextColor(COLOR_BATTERY_VOLTAGE);
    batteryVoltageSprite->drawString(String(tensionStr), 0, 0, 7);

    if (battery.GetLowBatStarted() > 0)
    {
        batteryVoltageSprite->drawRect(0, 25, 140, 10, TFT_GREEN);
        batteryVoltageSprite->fillRect(2, 27, map(esp_timer_get_time() - battery.GetLowBatStarted(), 0, battery.GetPeakSurgeDelay(), 0, 136), 8, TFT_GREEN);
    }

    drawManager.RequestDraw(4, 94, 140, 60, batteryVoltageSprite, nullptr, nullptr);
}

void Screen::drawSysBattery()
{
    sysBatterySprite->createSprite(60, 20);
    sysBatterySprite->fillSprite(BG_SYSTEM);
    sysBatterySprite->setTextColor(TFT_BROWN);

    sysBatterySprite->drawString(String((float)amoled.getBattVoltage()/1000, 2), 0, 0, 4);

    drawManager.RequestDraw(amoled.width() - 60, 4, 60, 20, sysBatterySprite, nullptr, nullptr);
}

bool Screen::isHidden()
{
    return hidden;
}
