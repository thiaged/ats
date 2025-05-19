#include "ScreenWave.h"

ScreenWave::ScreenWave(
    EnergySource &pUtilitySource,
    EnergySource &pSolarSource,
    EnergySource **pActiveSource,
    EnergySource **pDefinedSource,
    EnergySource **pUserDefinedSource,
    BatterySource &pBattery,
    DrawManager &pDrawManager,
    TFT_eSPI &pDisplay,
    LilyGo_Class &pAmoled,
    int pCyclesToCapture,
    int pSamplesPerCycle,
    bool *pUserSourceLocked,
    Preferences &pConfigPreferences
)
    : Screen(pUtilitySource, pSolarSource, pActiveSource, pDefinedSource, pUserDefinedSource, pBattery, pDrawManager, pDisplay,
        pAmoled, pUserSourceLocked, pConfigPreferences)
{
    cyclesToCapture = pCyclesToCapture;
    samplesPerCycle = pSamplesPerCycle;
    totalSamples = samplesPerCycle * cyclesToCapture;
    waveSprite = new TFT_eSprite(&display);
    statusBarSprite = new TFT_eSprite(&display); // Create a sprite for the status bar
}

void ScreenWave::Render()
{
    Screen::Render();
    drawWave();
}

bool ScreenWave::IsSyncronized()
{
    return isSyncronized;
}

void ScreenWave::SetSyncronized(bool value)
{
    isSyncronized = value;
}

void ScreenWave::DefineSyncronizedStatus()
{
    int phaseDiff = utilitySource.GetSensorValue() - solarSource.GetSensorValue();
    isSyncronized = abs(phaseDiff) < syncThreshold && utilitySource.GetWaveDirection() == solarSource.GetWaveDirection();
}

void ScreenWave::drawWave()
{
    waveSprite->createSprite(screenW, screenH);

    // uint64_t start = esp_timer_get_time(); // Obtém o tempo atual em µs

    int yOffset = 4;
    int xOffset = 4;

    waveSprite->fillSprite(BG_WAVE);

    std::deque<unsigned long long> solarBuffer = solarSource.GetBufferDraw();
    std::deque<unsigned long long> utilityBuffer = utilitySource.GetBufferDraw();

    uint16_t endPointUtilityX = 0;
    uint16_t endPointUtilityY = utilityBuffer.at(0);
    uint16_t endPointSolarX = 0;
    uint16_t endPointSolarY = solarBuffer.at(0);

    // Desenhar a onda na tela usando os dados do buffer
    for (int i = 1; i < solarBuffer.size(); i++)
    {
        if (i >= utilityBuffer.size()) {
            continue;
        }
        // if (printValues) Serial.println(utilitySource->GetWaveBufferDrawableValue(i));
        // Mapeia a posição no eixo X (largura do sprite)
        int x1 = map(endPointUtilityX, 0, totalSamples, xOffset, screenW - xOffset);
        int x2 = map(i, 0, totalSamples, xOffset, screenW - xOffset);

        // Mapeia a amplitude da onda para o eixo Y (altura do sprite)
        int y1 = map(endPointUtilityY, sourceOffThreshold, ADC_SCALE, screenH - yOffset, yOffset);
        int y2 = map(utilityBuffer.at(i), sourceOffThreshold, ADC_SCALE, screenH - yOffset, yOffset);

        endPointUtilityX = i;
        endPointUtilityY = utilityBuffer.at(i);

        // Desenha a linha conectando dois pontos sucessivos da onda
        waveSprite->drawLine(x1, y1, x2, y2, COLOR_WAVE_UTILITY);

        // solar
        //  Mapeia a posição no eixo X (largura do sprite)
        x1 = map(endPointSolarX, 0, totalSamples, xOffset, screenW - xOffset);
        x2 = map(i, 0, totalSamples, xOffset, screenW - xOffset);

        // Mapeia a amplitude da onda para o eixo Y (altura do sprite)
        y1 = map(endPointSolarY, sourceOffThreshold, ADC_SCALE, screenH - yOffset, yOffset);
        y2 = map(solarBuffer.at(i), sourceOffThreshold, ADC_SCALE, screenH - yOffset, yOffset);

        endPointSolarX = i;
        endPointSolarY = solarBuffer.at(i);

        // Desenha a linha conectando dois pontos sucessivos da onda
        waveSprite->drawLine(x1, y1, x2, y2, COLOR_WAVE_SOLAR);
    }

    // printValues = false;

    drawManager.RequestDraw(screenX, screenY, screenW, screenH, waveSprite, &utilitySource, &solarSource);

    // uint64_t end = esp_timer_get_time(); // Obtém o tempo depois da execução
}