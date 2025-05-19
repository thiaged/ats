#pragma once
#include <adapter/output/Screen.h>

class ScreenWave : public Screen
{
private:

    TFT_eSprite* waveSprite;
    TFT_eSprite* statusBarSprite;
    // config wave sprite
    // const int waveW = 350;
    // const int waveH = 160;
    // const int waveX = 180;
    // const int waveY = 40;
    int cyclesToCapture;
    int samplesPerCycle;
    int totalSamples;
    const float sampleInterval = 1.0 / FREQUENCY * 1000000 / samplesPerCycle; // Intervalo entre amostras para 60Hz
    const int sourceOffThreshold = 700; // Valor de threshold para detectar queda de rede

    String statusMsg = "";

    bool isSyncronized = false; //iniciar false
    int SincronizeWaitingLimitMicroseconds = 1000000; //value in microseconds
    const int syncThreshold = 5;

    void drawWave();

    void drawStatusBar();

public:
    ScreenWave(
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
    );

    virtual ~ScreenWave() = default;

    void Render() override;

    bool IsSyncronized();
    void SetSyncronized(bool value);
    void DefineSyncronizedStatus();

    void StopSyncWatch();
    void ClearStatusBar();
    void TransferTo(EnergySource &source, bool sincronized, bool syncWatchTransfer = false);
    void StopScreenWave();

};