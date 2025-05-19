#include "BatterySource.h"

bool BatterySource::IsLowBattery()
{
    if (bmsComunicationOn) {
        return batteryPercentage < batteryConfigMinPercentage;
    }

    return batteryVoltage < batteryConfigMin;
}

bool BatterySource::IsEmpty()
{
    return batteryVoltage < batteryVoltageMin;
}

void BatterySource::SetBatteryVoltage(double value)
{
    batteryVoltage = value;
}

void BatterySource::SetBatteryPercentage(double value)
{
    batteryPercentage = value;
}

void BatterySource::SetBmsComunicationOn(bool value)
{
    bool changed = value != bmsComunicationOn;
    bmsComunicationOn = value;
    if (changed)
    {
        configPreferences.begin("config", false);
        configPreferences.putBool("bms-on", value); //maximo 15 char on name
        configPreferences.end();
    }

}

void BatterySource::SetBatteryConfigMin(double value)
{
    bool changed = value != batteryConfigMin;
    batteryConfigMin = value;
    if (changed)
    {
        configPreferences.begin("config", false);
        configPreferences.putDouble("bMin", value); //maximo 15 char on name
        configPreferences.end();
    }
}

void BatterySource::SetBatteryConfigMax(double value)
{
    bool changed = value != batteryConfigMax;
    batteryConfigMax = value;
    if (changed)
    {
        configPreferences.begin("config", false);
        configPreferences.putDouble("bMax", value); //maximo 15 char on name
        configPreferences.end();
    }
}

void BatterySource::SetBatteryConfigMinPercentage(int value)
{
    bool changed = value != batteryConfigMinPercentage;
    batteryConfigMinPercentage = value;
    if (changed)
    {
        configPreferences.begin("config", false);
        configPreferences.putInt("bMinPercent", value); //maximo 15 char on name
        configPreferences.end();
    }
}

void BatterySource::SetBatteryConfigMaxPercentage(int value)
{
    bool changed = value != batteryConfigMaxPercentage;
    batteryConfigMaxPercentage = value;
    if (changed)
    {
        configPreferences.begin("config", false);
        configPreferences.putInt("bMaxPercent", value); //maximo 15 char on name
        configPreferences.end();
    }
}

void BatterySource::SetBatteryVoltageMin(double value)
{
    batteryVoltageMin = value;
}

void BatterySource::SetBatteryVoltageMax(double value)
{
    batteryVoltageMax = value;
}

void BatterySource::UpdateLowBatteryStarted()
{
    lowBatteryStarted = esp_timer_get_time();
}

uint64_t BatterySource::GetLowBatStarted()
{
    return lowBatteryStarted;
}

uint64_t BatterySource::GetPeakSurgeDelay()
{
    return peakSurgeDelay;
}

bool BatterySource::GetBmsComunicationOn()
{
    return bmsComunicationOn;
}

bool BatterySource::LowBatteryDelayEnded()
{
    return esp_timer_get_time() - lowBatteryStarted > peakSurgeDelay;
}

void BatterySource::ResetLowbatteryStarted()
{
    lowBatteryStarted = 0;
}

void BatterySource::funcSensorReadTask()
{
    // Serial.println("Battery sensor read task");
    if (bmsComunicationOn == true)
    {
        readBatteryBMS();
    }
    else
    {
        readBatteryResistorDivisor();
    }

    portYIELD_FROM_ISR(pdFALSE); // Garante que a task vai ser executada imediatamente
    // vTaskDelay(100 / portTICK_PERIOD_MS);
}

void BatterySource::sensorReadTask(void *pvParameters)
{
    BatterySource* instance = static_cast<BatterySource*>(pvParameters);

    if (instance == nullptr)
    {
        Serial.println("Erro ao obter a instância do BatterySource");
        return;
    }

    while (true)
    {
        if ((*instance->updatingFirmware) == false)
        {
            instance->funcSensorReadTask();
        }
        vTaskDelay((instance->sampleIntervalBattery / 1000) / portTICK_PERIOD_MS); // Aguarda X ms antes de repetir a leitura
    }
}

void BatterySource::readBatteryBMS()
{
    // Envia o comando para ler tensão e porcentagem da BMS
    enviarComandoLeitura();
    delay(150);

    if (bmsComunicationFailures > 5) {
        SetBmsComunicationOn(false);
    }

    // Lê e interpreta a resposta
    if (rs485.available())
    {
        uint8_t resposta[30];
        int bytesRecebidos = rs485.readBytes(resposta, 30);

        if (bytesRecebidos > 0 && validarChecksum(resposta, bytesRecebidos))
        {
            interpretarResposta(resposta);
        }
        else
        {
            bmsComunicationFailures++;
            Serial.println("Erro na leitura ou checksum inválido");
        }
    } else {
        bmsComunicationFailures++;
        Serial.println("Sem resposta da BMS");
    }
}

void BatterySource::enviarComandoLeitura()
{
    uint8_t comando[] = {0xDD, 0xA5, 0x03, 0x00, 0xFF, 0xFD, 0x77};
    rs485.write(comando, sizeof(comando));
    Serial.println("Comando de leitura enviado");
}

bool BatterySource::validarChecksum(uint8_t *data, int len)
{
    // Implemente a lógica de validação de checksum de acordo com a documentação
    // (soma dos bytes, complemento +1)
    return true; // Retorne verdadeiro se o checksum for válido
}

void BatterySource::interpretarResposta(uint8_t *data)
{
    uint16_t tensaoRaw = (data[0] << 8) | data[1]; // Converte os dois primeiros bytes para uint16_t
    uint8_t porcentagemRaw = data[10];             // Lê o 11º byte como porcentagem

    // Converte valores brutos para valores reais de tensão e porcentagem
    batteryVoltage = tensaoRaw / 100.0; // Dividir por 100 para converter de 10mV para V
    batteryPercentage = porcentagemRaw; // Porcentagem direto de 0 a 100
}

BatterySource::BatterySource(
    DynamicAnalogBuffer &pBatteryReadBuffer,
    Preferences &pConfigPreferences,
    bool *pUpdatingFirmware
)
    : batteryReadBuffer(pBatteryReadBuffer), configPreferences(pConfigPreferences)
{
    updatingFirmware = pUpdatingFirmware;
}

void BatterySource::Init()
{
    pinMode(BATTERY_SENSOR_PIN, INPUT);
    rs485.begin(9600, SERIAL_8N1, 44, 43); // RX, TX
    // rs485->begin(9600);
    // rs485->listen();  GPIO44 - UORXD and GPIO43 - UOTXD
    Serial.println("BMS Comunication started");
    startSensorReadTask();

    configPreferences.begin("config", true);

    batteryConfig.batteryCalibrationValue = configPreferences.getFloat("battery-calib", batteryConfig.batteryCalibrationValue);
    bmsComunicationOn = configPreferences.getBool("bms-on", false);
    batteryConfigMin = configPreferences.getDouble("bMin", 23.50);
    batteryConfigMax = configPreferences.getDouble("bMax", 27.00);
    batteryConfigMinPercentage = configPreferences.getInt("bMinPercent", 40);
    batteryConfigMaxPercentage = configPreferences.getInt("bMaxPercent", 60);

    configPreferences.end();
}

void BatterySource::startSensorReadTask()
{
    xTaskCreatePinnedToCore(
        sensorReadTask,        // Função da task
        "ReadBatterySensorTask",  // Nome da task
        4096,                  // Tamanho da stack (bytes)
        this,                  // Passa a instância como parâmetro
        2,                     // Prioridade da task
        &batterySensorTaskHandle, // Handle da task (opcional)
        1                      // Núcleo onde a task será fixada (0 ou 1)
    );
}

void BatterySource::readBatteryResistorDivisor()
{
    // Leitura do valor analógico (entre 0 e 4095)
    int adcValue = analogReadMilliVolts(BATTERY_SENSOR_PIN);
    if (adcValue < 700) adcValue = 0;

    batteryReadBuffer.AddReading(adcValue);
    batteryVoltage = (batteryReadBuffer.GetMean() / 4096.0 * 3.33) * ((BATTERY_SENSOR_R1 + BATTERY_SENSOR_R2) / BATTERY_SENSOR_R2) * batteryConfig.batteryCalibrationValue;

}

double BatterySource::GetBatteryConfigMin()
{
    return batteryConfigMin;
}

unsigned int BatterySource::GetBatteryConfigMinPercentage()
{
    return batteryConfigMinPercentage;
}

unsigned int BatterySource::GetBatteryConfigMaxPercentage()
{
    return batteryConfigMaxPercentage;
}

double BatterySource::GetBatteryConfigMax()
{
    return batteryConfigMax;
}

double BatterySource::GetBatteryVoltage()
{
    return batteryVoltage;
}

double BatterySource::GetBatteryPercentage()
{
    return batteryPercentage;
}

double BatterySource::GetBatteryVoltageMin()
{
    return batteryVoltageMin;
}

double BatterySource::GetBatteryVoltageMax()
{
    return batteryVoltageMax;
}

bool BatterySource::IsHighBattery()
{
    if (bmsComunicationOn) {
        return batteryPercentage > batteryConfigMaxPercentage;
    }

    return batteryVoltage > batteryConfigMax;
}
