#include "BatterySource.h"

bool BatterySource::IsLowBattery()
{
    if (BMS.isConnected)
    {
        return batteryPercentage < batteryConfigMinPercentage;
    }

    return batteryVoltage < batteryConfigMin;
}

bool BatterySource::IsEmpty()
{
    if (BMS.isConnected)
    {
        return batteryPercentage < 5; // Considered empty when below 5%
    }
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
        configPreferences.putBool("bms-on", value); // maximo 15 char on name
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
        configPreferences.putDouble("bMin", value); // maximo 15 char on name
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
        configPreferences.putDouble("bMax", value); // maximo 15 char on name
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
        configPreferences.putInt("bMinPercent", value); // maximo 15 char on name
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
        configPreferences.putInt("bMaxPercent", value); // maximo 15 char on name
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

BMSData BatterySource::GetBms()
{
    return BMS;
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

    readBatteryResistorDivisor();

    portYIELD_FROM_ISR(pdFALSE); // Garante que a task vai ser executada imediatamente
    // vTaskDelay(100 / portTICK_PERIOD_MS);
}

void BatterySource::sensorReadTask(void *pvParameters)
{
    BatterySource *instance = static_cast<BatterySource *>(pvParameters);

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

BatterySource::BatterySource(
    DynamicAnalogBuffer &pBatteryReadBuffer,
    Preferences &pConfigPreferences,
    HardwareSerial &pSerial,
    bool *pUpdatingFirmware)
    : batteryReadBuffer(pBatteryReadBuffer), configPreferences(pConfigPreferences), RS485Serial(pSerial)
{
    updatingFirmware = pUpdatingFirmware;
}

void BatterySource::Init()
{
    pinMode(BATTERY_SENSOR_PIN, INPUT);
    RS485Serial.begin(RS485_BAUD, SERIAL_8N1, RS485_RX, RS485_TX);
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
        sensorReadTask,           // Função da task
        "ReadBatterySensorTask",  // Nome da task
        4096,                     // Tamanho da stack (bytes)
        this,                     // Passa a instância como parâmetro
        2,                        // Prioridade da task
        &batterySensorTaskHandle, // Handle da task (opcional)
        1                         // Núcleo onde a task será fixada (0 ou 1)
    );
}

void BatterySource::readBatteryResistorDivisor()
{
    // Leitura do valor analógico (entre 0 e 4095)
    int adcValue = analogReadMilliVolts(BATTERY_SENSOR_PIN);
    if (adcValue < 700)
        adcValue = 0;

    batteryReadBuffer.AddReading(adcValue);
    batteryVoltage = (batteryReadBuffer.GetMean() / 4096.0 * 3.33) * ((BATTERY_SENSOR_R1 + BATTERY_SENSOR_R2) / BATTERY_SENSOR_R2) * batteryConfig.batteryCalibrationValue;
}

void BatterySource::readBatteryBMS()
{
    if ((millis() - lastBmsRequestTime) > 2000)
    {
        sendCommand(REQUEST_BASIC_INFO, REQUEST_BASIC_INFO_SIZE);
        lastBmsRequestTime = millis();
    }

    // Lê dados disponíveis na porta serial
    while (RS485Serial.available() && bufferIndex < MAX_BUFFER_SIZE)
    {
        receiveBuffer[bufferIndex++] = RS485Serial.read();
    }

    // Se recebeu dados, processa-os
    if (bufferIndex > 0)
    {
        // Procura pelo início do pacote (0xDD, 0xA5)
        for (int i = 0; i < bufferIndex - 1; i++)
        {
            if (receiveBuffer[i] == 0xDD && receiveBuffer[i + 1] == 0xA5)
            {
                // Encontrou o início do pacote, processa os dados
                processReceivedData();

                // Limpa o buffer após processamento
                bufferIndex = 0;
                break;
            }
        }

        // Se o buffer está cheio mas não encontrou um pacote válido, limpa-o
        if (bufferIndex >= MAX_BUFFER_SIZE)
        {
            bufferIndex = 0;
            Serial.println("Buffer cheio, limpando");
        }
    }

    // Verifica timeout de conexão
    checkConnectionTimeout();
}

void BatterySource::processReceivedData()
{
    // Verifica se os dados recebidos têm o formato correto
    // Este é um exemplo simplificado e deve ser adaptado conforme o protocolo real da JK BMS

    // Verifica o cabeçalho e o tamanho mínimo
    if (bufferIndex < 10 || receiveBuffer[0] != 0xDD || receiveBuffer[1] != 0xA5)
    {
        Serial.println("Dados inválidos recebidos");
        return;
    }

    // Extrai o comprimento dos dados
    int dataLength = receiveBuffer[2];

    // Verifica se recebemos dados suficientes
    if (bufferIndex < dataLength + 7)
    {
        Serial.println("Pacote incompleto");
        return;
    }

    // Verifica o checksum (implementação simplificada)
    // Na implementação real, você deve calcular e verificar o checksum conforme o protocolo da JK BMS

    // Exemplo de parsing (os offsets e multiplicadores devem ser ajustados conforme protocolo real)
    // Nota: Este é um exemplo genérico e deve ser adaptado para o protocolo específico da JK BMS
    BMS.totalVoltage = (receiveBuffer[4] * 256 + receiveBuffer[5]) / 100.0;
    BMS.current = ((int16_t)(receiveBuffer[6] * 256 + receiveBuffer[7])) / 100.0;
    BMS.power = BMS.totalVoltage * BMS.current;
    BMS.temperature = (receiveBuffer[8] - 40); // Exemplo de conversão de temperatura
    BMS.soc = receiveBuffer[10];

    // Atualiza o timestamp da última resposta válida
    BMS.lastResponseTime = millis();
    BMS.isConnected = true;

    Serial.println("Dados processados com sucesso");
}

void BatterySource::sendCommand(const byte *command, int length)
{
    // Limpa o buffer de recepção antes de enviar um novo comando
    while (RS485Serial.available())
    {
        RS485Serial.read();
    }

    // Envia o comando
    RS485Serial.write(command, length);
}

void BatterySource::checkConnectionTimeout()
{
    // Se não receber resposta por mais de 5 segundos, considera desconectado
    if (BMS.isConnected && (millis() - BMS.lastResponseTime > 1000))
    {
        BMS.isConnected = false;
        Serial.println("Timeout de conexão com a BMS");
    }
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
    if (BMS.isConnected)
    {
        return batteryPercentage > batteryConfigMaxPercentage;
    }

    return batteryVoltage > batteryConfigMax;
}
