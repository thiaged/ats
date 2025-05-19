#pragma once
#include <core/domain/EnergySource.h>
#include <adapter/output/ScreenWave.h>

class EnergyInput
{
private:
    EnergySource &utilitySource;
    EnergySource &solarSource;
    ScreenWave &screenWave;
    int sampleInterval; //167 µs - default 833 for 20 samples per cycle

    bool *updatingFirmware;

    esp_timer_handle_t timer_sensor_zmpt;
    esp_timer_handle_t timer_frequecence_check;
    // TaskHandle_t zmptSensorTaskHandle = NULL;

    void funcSensorReadTask();
    static void sensorReadTask(void *pvParameters);
    void startSensorReadTask();

public:
    EnergyInput(
        EnergySource &pUtilitySource,
        EnergySource &pSolarSource,
        ScreenWave &pScreenWave,
        int pSampleInterval,
        bool *pUpdatingFirmware
    );
    void Init();
    virtual ~EnergyInput() = default;

    EnergySource *GetUtilitySource();
    EnergySource *GetSolarSource();

};