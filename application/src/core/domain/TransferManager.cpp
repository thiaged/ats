#include "TransferManager.h"
#include <adapter/output/ScreenWave.h>
#include <driver/rtc_io.h>

TransferManager::TransferManager(
    EnergySource **pActiveSource,
    EnergySource **pDefinedSource,
    EnergySource **pUserDefinedSource,
    EnergyInput &pEnergyInput,
    ScreenManager &pScreenManager,
    ScreenWave &pScreenWave,
    BuzzerManager &pBuzzerManager,
    BatterySource &pBattery,
    bool *pUserSourceLocked,
    bool *pUpdatingFirmware,
    unsigned long *pInactivityTime,
    bool &pWaitForSync
)
    : energyInput(pEnergyInput), screenManager(pScreenManager), screenWave(pScreenWave), buzzerManager(pBuzzerManager),
        battery(pBattery), waitForSync(pWaitForSync)
{
    activeSource = pActiveSource;
    definedSource = pDefinedSource;
    userDefinedSource = pUserDefinedSource;
    userSourceLocked = pUserSourceLocked;
    updatingFirmware = pUpdatingFirmware;
    inactivityTime = pInactivityTime;
}

void TransferManager::Init()
{
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(UTILITY_OFF_LED_PIN, OUTPUT);

    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(UTILITY_OFF_LED_PIN, LOW);

    startNoBreakTask();
}

void TransferManager::TransferToDefined(bool pTransferSincronized, bool pSyncWatchTransfer)
{
    if (transfering == true && !pSyncWatchTransfer)
    {
        return;
    }

    Serial.println("Transfering to " + (*definedSource)->GetTypePrintable());
    transfering = true;

    if ((*activeSource)->GetType() == (*definedSource)->GetType())
    {
        transfering = false;
        return;
    }

    if (!(*definedSource)->IsOnline() && (*activeSource)->IsOnline())
    {
        Serial.println("Source " + String((*definedSource)->GetType()) + " off line, aborting jump");
        transfering = false;
        definedSource = activeSource;
        return;
    }

    if (pSyncWatchTransfer == false && millis() < lastTransfered + delayTransferProtect)
    {
        Serial.println("Fast jump protect");
        transfering = false;
        return;
    }

    int cur = micros();
    if (waitForSync == true && pTransferSincronized == true && !screenWave.IsSyncronized())
    {
        screenManager.GoToScreen(&screenWave);
        startSyncWatchTask();
        return;
    }

    switch ((*definedSource)->GetType())
    {
    case SourceType::SOLAR_SOURCE_TYPE:
        digitalWrite(RELAY_PIN, HIGH);
        break;
    case SourceType::UTILITY_SOURCE_TYPE:
        digitalWrite(RELAY_PIN, LOW);
        digitalWrite(UTILITY_OFF_LED_PIN, LOW);
        break;
    default:
        Serial.println("inválid source type");
        break;
    }

    lastTransfered = millis();
    *activeSource = *definedSource;
    transfering = false;
    // definedScreen = userDefinedScreen; TODO:
}

void TransferManager::startSyncWatchTask()
{
    if (syncWatchTaskHandle == NULL)
    {
        xTaskCreatePinnedToCore(
            syncWatchTask, "SyncWatchTask", 2048, this, 2, &syncWatchTaskHandle, 0);
    }
}

void TransferManager::funcSyncWatchTask(void *pvParameters)
{
    Serial.println("syncwatch task iniciada");
    unsigned long waitSyncLimit = millis() + 120000;
    *inactivityTime = millis();

    while (true && !(*updatingFirmware))
    {
        if (!(*definedSource)->IsOnline()) {
            Serial.println("defined source became offline");
            *definedSource = *activeSource;
            transfering = false;
            break;
        }

        if (! waitForSync) {
            Serial.println("waitForSync is disabled");
            TransferToDefined(false, true);
            screenManager.PreviousScreen();
            break;
        }

        if (screenWave.IsSyncronized())
        {
            Serial.println("sine wave is synced.");

            TransferToDefined(false, true);
            screenManager.PreviousScreen();
            break;
        }

        if (millis() > waitSyncLimit)
        {
            Serial.println("Long time waiting sync.");

            TransferToDefined(false, true);
            screenManager.PreviousScreen();
            break;
        }

        if ((millis() - debounceSound) > 900) {
            buzzerManager.PlaySyncSound();
            debounceSound = millis();
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    syncWatchTaskHandle = NULL;
    vTaskDelete(NULL);
}

void TransferManager::syncWatchTask(void *pvParameters)
{
    TransferManager* instance = static_cast<TransferManager*>(pvParameters);
    if (instance == nullptr)
    {
        Serial.println("Erro ao obter a instância do TransferManager");
        return;
    }
    instance->funcSyncWatchTask(pvParameters);
}

void TransferManager::startNoBreakTask()
{
    if (noBreakTaskHandle == NULL)
    {
        xTaskCreatePinnedToCore(
            noBreakTask, "NoBreakTask", 4096, this, 2, &noBreakTaskHandle, 0);
    }
}

void TransferManager::funcNoBreakTask(void *pvParameters)
{
    Serial.println("nobreak task iniciada");
    while (true)
    {
        if (transfering == true || *updatingFirmware == true)
        {
            // Serial.println("transfering");
            vTaskDelay(10 / portTICK_PERIOD_MS);
            continue;
        }

        if (!energyInput.GetSolarSource()->IsOnline() || !energyInput.GetUtilitySource()->IsOnline() && !screenWave.IsSyncronized())
        {
            // Serial.println("synced by source off");
            screenWave.SetSyncronized(true);
        }

        // esta na utility e a utility ficou off e solar esta ok
        if ((*activeSource)->GetType() == SourceType::UTILITY_SOURCE_TYPE &&
            energyInput.GetUtilitySource()->IsOnline() == false &&
            transfering == false &&
            energyInput.GetSolarSource()->IsOnline() == true)
        {
            // jump to solar
            Serial.println("utility ficou off, pula pra solar");
            *definedSource = energyInput.GetSolarSource();
            TransferToDefined(false);
            continue;
        }

        // esta na solar e a solar ficou off e utility esta ok
        if ((*activeSource)->GetType() == SourceType::SOLAR_SOURCE_TYPE &&
            energyInput.GetSolarSource()->IsOnline() == false &&
            transfering == false &&
            energyInput.GetUtilitySource()->IsOnline() == true)
        {
            // jump to utility
            Serial.println("solar ficou off, pula pra utility");
            *definedSource = energyInput.GetUtilitySource();
            TransferToDefined(false);
            continue;
        }

        if (*userDefinedSource != nullptr) {
            if ((*activeSource)->GetType() != (*userDefinedSource)->GetType() &&
            transfering == false && (*userDefinedSource)->IsOnline())
            {
                if ((*userDefinedSource)->GetType() == energyInput.GetSolarSource()->GetType() && battery.IsEmpty())
                {
                    continue;
                }
                *definedSource = *userDefinedSource;
                TransferToDefined(true);
                continue;
            }
        }


        if (
            (*activeSource)->GetType() == energyInput.GetSolarSource()->GetType() &&
            battery.IsLowBattery() &&
            energyInput.GetUtilitySource()->IsOnline() &&
            (*userSourceLocked == false || battery.IsEmpty())
        )
        {
            if (battery.IsEmpty())
            {
                *definedSource = energyInput.GetUtilitySource();
                TransferToDefined(false);
                continue;
            }

            if (battery.GetLowBatStarted() == 0)
            {
                battery.UpdateLowBatteryStarted();
            }
            else if (battery.LowBatteryDelayEnded())
            {
                *definedSource = energyInput.GetUtilitySource();
                TransferToDefined(true);
                continue;
            }
        }
        else
        {
            battery.ResetLowbatteryStarted();
        }

        if (
            (*activeSource)->GetType() == energyInput.GetUtilitySource()->GetType() &&
            battery.IsHighBattery() &&
            energyInput.GetSolarSource()->IsOnline() &&
            *userSourceLocked == false
        )
        {
            *definedSource = energyInput.GetSolarSource();
            TransferToDefined(true);
            continue;
        }

        if (!energyInput.GetSolarSource()->IsOnline() && !energyInput.GetUtilitySource()->IsOnline() && (*activeSource)->GetType() == energyInput.GetSolarSource()->GetType())
        {
            //vai para rede padrao
            *definedSource = energyInput.GetUtilitySource();
            TransferToDefined(true);
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void TransferManager::noBreakTask(void *pvParameters)
{
    TransferManager* instance = static_cast<TransferManager*>(pvParameters);
    if (instance == nullptr)
    {
        Serial.println("Erro ao obter a instância do TransferManager");
        return;
    }
    instance->funcNoBreakTask(pvParameters);
}
