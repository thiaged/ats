#pragma once
#include "EnergySource.h"
#include <adapter/input/EnergyInput.h>
#include <adapter/output/ScreenManager.h>
#include <adapter/output/BuzzerManager.h>

#define RELAY_PIN 39
#define UTILITY_OFF_LED_PIN 41

class TransferManager
{
private:
    EnergyInput &energyInput;
    EnergySource **activeSource;
    EnergySource **definedSource;
    EnergySource **userDefinedSource;
    ScreenManager &screenManager;
    ScreenWave &screenWave;
    BuzzerManager &buzzerManager;
    BatterySource &battery;
    Logger &logger;
    bool *userSourceLocked;
    bool *updatingFirmware;
    unsigned long *inactivityTime;
    bool &waitForSync;

    bool transfering = false;
    const int delayTransferProtect = 1000;
    unsigned long lastTransfered = 0;

    unsigned long debounceSound = 0;

    TaskHandle_t syncWatchTaskHandle = NULL;
    TaskHandle_t noBreakTaskHandle = NULL;
    unsigned long noBreakLastHeartbeat = 0;

    void startSyncWatchTask();
    void funcSyncWatchTask(void *pvParameters);
    static void syncWatchTask(void* pvParameters);

    void startNoBreakTask();
    void funcNoBreakTask(void *pvParameters);
    static void noBreakTask(void* pvParameters);

public:
    TransferManager(
        EnergySource **pActiveSource,
        EnergySource **pDefinedSource,
        EnergySource **pUserDefinedSource,
        EnergyInput &pEnergyInput,
        ScreenManager &pScreenManager,
        ScreenWave &screenWave,
        BuzzerManager &pBuzzerManager,
        BatterySource &pBattery,
        Logger &pLogger,
        bool *pUserSourceLocked,
        bool *pUpdatingFirmware,
        unsigned long *pInactivityTime,
        bool &pWaitForSync
    );

    virtual ~TransferManager() = default;

    void Init();
    void TransferToDefined(bool pTransferSincronized, bool pSyncWatchTransfer = false);
    void SetDefinedSource(EnergySource &source);

    // Task monitoring
    bool IsNoBreakTaskHealthy() const;
    void EnsureNoBreakTaskRunning();

};