#include "ScreenBMS.h"

ScreenBMS::ScreenBMS(
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
    bmsSprite = new TFT_eSprite(&display);
}

void ScreenBMS::Render()
{
    Screen::Render();
    if ((millis() - slowScreenAnimBms) > 1000) // Update every second
    {
        slowScreenAnimBms = millis();
        drawBMSData();
    }
}

void ScreenBMS::drawBMSData()
{
    if (!bmsSprite) return;

    bmsSprite->createSprite(screenW, screenH);
    bmsSprite->fillSprite(BG_WAVE);
    bmsSprite->setTextColor(TFT_WHITE);

    drawBMSHeader();
    drawBMSStats();

    // Draw cell voltages if BMS is connected and has data
    if (battery.GetBmsConnected() && battery.GetBmsTotalStrings() > 0) {
        drawCellVoltages(); // Agora desenha DENTRO do bmsSprite
    }

    drawManager.RequestDraw(screenX, screenY, screenW, screenH, bmsSprite, nullptr, nullptr);
}

void ScreenBMS::drawBMSHeader()
{
    if (!bmsSprite) return;

    bmsSprite->setTextColor(TFT_CYAN);
    bmsSprite->drawString("BMS Status", 10, 5, 4);

    bmsSprite->setTextColor(battery.GetBmsConnected() ? TFT_GREEN : TFT_RED);
    bmsSprite->drawString(battery.GetBmsConnected() ? "ONLINE" : "OFFLINE", 150, 5, 4);
}

void ScreenBMS::drawBMSStats()
{
    bmsSprite->setTextColor(TFT_WHITE);
    int yPos = 30;
    int lineHeight = 28; // Altura da fonte 4 + espaçamento

    char buffer[32];

    // Linha 1: Corrente
    snprintf(buffer, sizeof(buffer), "Current: %.2fA", battery.GetBmsCurrent());
    bmsSprite->drawString(buffer, 10, yPos, 4);
    yPos += lineHeight;

    // Linha 2: Potência
    snprintf(buffer, sizeof(buffer), "Power: %.1fW", battery.GetBmsPower());
    bmsSprite->drawString(buffer, 10, yPos, 4);
    yPos += lineHeight;

    // Linha 1 direita: SOC
    snprintf(buffer, sizeof(buffer), "SOC: %d%%", battery.GetBmsSoc());
    bmsSprite->drawString(buffer, 220, 30, 4);
}

void ScreenBMS::drawCellVoltages()
{
    if (battery.GetBmsTotalStrings() == 0) return;

    // --- Define a área para as células (lado direito) ---
    int cellStartX = 10; // Começa à direita dos dados principais
    int cellWidth = 180;
    int cellStartY = 92;  // Alinhado com o topo dos dados

    // --- Determina a fonte e altura da linha baseada no número de células ---
    int lineHeight = 28; // Altura aproximada da fonte 4

    // --- Desenha todas as células ---
    int yPos = cellStartY;
    int xPos = cellStartX;
    for (int i = 1; i <= battery.GetBmsTotalStrings(); i++) {
        if (i > 8) break; // Limita a 8 células para evitar overflow
        float cellVoltage = battery.GetBmsCellVoltage(i);

        // Define cor do texto baseado na tensão
        uint16_t textColor = TFT_WHITE;
        if (i == battery.GetBms().highestCellIndex) {
            textColor = TFT_BLUE; // Cor especial para a célula mais alta
        }

        if (i == battery.GetBms().lowestCellIndex) {
            textColor = TFT_RED; // Cor especial para a célula mais baixa
        }

        bmsSprite->setTextColor(textColor);

        // Formato: "1 - 3.456V"
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%d    %.3fV", i, cellVoltage);

        // Desenha a célula
        bmsSprite->drawString(buffer, xPos, yPos, 4);

        if (i % 2 == 0) {
            xPos = cellStartX;
            yPos += lineHeight;
        } else {
            xPos = cellStartX + cellWidth;
        }

    }
}