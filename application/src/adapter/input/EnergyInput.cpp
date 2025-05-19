#include "EnergyInput.h"
#include <adapter/output/ScreenWave.h>

void EnergyInput::funcSensorReadTask()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    utilitySource.ProcessSensorValue(analogRead(utilitySource.GetSensorPin()));
    solarSource.ProcessSensorValue(analogRead(solarSource.GetSensorPin()));

    screenWave.DefineSyncronizedStatus();

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // Garante que a task vai ser executada imediatamente
    // vTaskDelay(10 / portTICK_PERIOD_MS);
}

void EnergyInput::sensorReadTask(void *pvParameters)
{
    EnergyInput *instance = static_cast<EnergyInput *>(pvParameters);
    if (instance == nullptr)
    {
        Serial.println("Erro ao obter a instância do EnergyInput");
        return;
    }

    if ((*instance->updatingFirmware) == true)
    {
        return;
    }

    instance->funcSensorReadTask();

}

void EnergyInput::startSensorReadTask()
{
    // Configurar o timer para chamar o callback a cada 167 µs
    const esp_timer_create_args_t timer_sensor_zmpt_args = {
        .callback = &sensorReadTask,      // Função de callback
        .arg = this,                      // Argumento (opcional)
        .name = "sensor_zmpt_read_timer", // Nome do timer
        .skip_unhandled_events = true
    };

    // Criar e iniciar o timer

    esp_timer_create(&timer_sensor_zmpt_args, &timer_sensor_zmpt);
    esp_timer_start_periodic(timer_sensor_zmpt, lround(sampleInterval)); // valor em microssegundos

    // xTaskCreate(sensorReadTask, "ReadZMPTSensorTask", 8100, this, 2, NULL);
}

EnergyInput::EnergyInput(
    EnergySource &pUtilitySource,
    EnergySource &pSolarSource,
    ScreenWave &pScreenWave,
    int pSampleInterval,
    bool *pUpdatingFirmware
)
    : utilitySource(pUtilitySource), solarSource(pSolarSource), screenWave(pScreenWave)
{
    sampleInterval = pSampleInterval;
    updatingFirmware = pUpdatingFirmware;
}

void EnergyInput::Init()
{
    pinMode(utilitySource.GetSensorPin(), INPUT);
    pinMode(solarSource.GetSensorPin(), INPUT);
    startSensorReadTask();
}

EnergySource *EnergyInput::GetUtilitySource()
{
    return &utilitySource;
}

EnergySource *EnergyInput::GetSolarSource()
{
    return &solarSource;
}
