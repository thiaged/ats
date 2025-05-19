#include "BatterySource.h"

bool BatterySource::IsLowBattery()
{
    if (bmsComunicationOn)
    {
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
        int delayTime = instance->sampleIntervalBattery / 1000;
        if (instance->bmsComunicationOn == true)
        {
            delayTime = 1000; // 1 segundo
        }
        vTaskDelay(delayTime / portTICK_PERIOD_MS); // Aguarda X ms antes de repetir a leitura
    }
}

BatterySource::BatterySource(
    DynamicAnalogBuffer &pBatteryReadBuffer,
    Preferences &pConfigPreferences,
    bool *pUpdatingFirmware)
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
    if (!readAndStore())
    {
        Serial.println("Failed to read BMS");
        bmsComunicationFailures++;
        if (bmsComunicationFailures > 5)
        {
            SetBmsComunicationOn(false);
        }
        return;
    }
}

bool BatterySource::readAndStore()
{
    const uint8_t regs[] = {0x79, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x89, 0x8A, 0x8B, 0x8C,
                            0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
                            0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
                            0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9,
                            0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
                            0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9,
                            0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0};
    sendReadRequest(regs, sizeof(regs));

    uint8_t hdr[4];
    if (rs485.readBytes(hdr, 4) != 4 || hdr[0] != 0x4E || hdr[1] != 0x57)
        return false;
    uint8_t len = hdr[3];
    uint8_t buf[128];
    if (rs485.readBytes(buf, len + 2) != len + 2)
        return false;

    uint16_t idx = 0;
    while (idx < len)
    {
        uint8_t code = buf[idx++];
        switch (code)
        {
        case 0x79:
        {
            BMS.cellCount = buf[idx++];
            for (uint8_t i = 0; i < BMS.cellCount && i < MAX_CELLS; i++)
            {
                BMS.cellVoltages[i] = (buf[idx] << 8) | buf[idx + 1];
                idx += 2;
            }
            break;
        }
        case 0x80:
            BMS.powerTubeTemp = (buf[idx++] << 8) | buf[idx++];
            break;
        case 0x81:
            BMS.boxTemp = (buf[idx++] << 8) | buf[idx++];
            break;
        case 0x82:
            BMS.batteryTemp = (buf[idx++] << 8) | buf[idx++];
            break;
        case 0x83:
            BMS.totalVoltage = (buf[idx++] << 8) | buf[idx++];
            batteryVoltage = BMS.totalVoltage * 0.01; // Convert to volts
            break;
        case 0x84:
            BMS.currentVal = (buf[idx++] << 8) | buf[idx++];
            break;
        case 0x85:
            BMS.soc = buf[idx++];
            batteryPercentage = BMS.soc; // Assuming SOC is in percentage
            break;
        case 0x86:
            BMS.tempSensorCount = buf[idx++];
            break;
        case 0x87:
            BMS.cycleCount = (buf[idx++] << 8) | buf[idx++];
            break;
        case 0x89:
            BMS.cycleCapacity = (uint32_t)buf[idx++] << 24 | (uint32_t)buf[idx++] << 16 |
                                (uint32_t)buf[idx++] << 8 | buf[idx++];
            break;
        case 0x8A:
            BMS.stringCount = (buf[idx++] << 8) | buf[idx++];
            break;
        case 0x8B:
            BMS.warningMask = (buf[idx++] << 8) | buf[idx++];
            break;
        case 0x8C:
            BMS.statusMask = (buf[idx++] << 8) | buf[idx++];
            break;
        case 0xC0:
            BMS.protocolVersion = buf[idx++];
            break;
        default:
            idx += (code >= 0x8E && code <= 0x9F) ? 2 : ((code >= 0xA0 && code <= 0xAF) ? 4 : (code >= 0xB0 && code <= 0xBF) ? 10
                                                                                                                             : 1);
            break;
        }
    }
    return true;
}

void BatterySource::sendReadRequest(const uint8_t *regs, size_t regCount)
{
    uint8_t frame[64] = {0};
    frame[0] = 0x4E;
    frame[1] = 0x57;
    frame[2] = 0x00;
    frame[3] = regCount;
    memcpy(frame + 8, regs, regCount);
    uint16_t crc = calcCRC(frame, 8 + regCount);
    frame[8 + regCount] = crc & 0xFF;
    frame[9 + regCount] = crc >> 8;
    rs485.write(frame, 10 + regCount);
}

uint16_t BatterySource::calcCRC(const uint8_t *buf, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i)
    {
        crc ^= buf[i];
        for (int j = 0; j < 8; j++)
            crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
    }
    return crc;
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
    if (bmsComunicationOn)
    {
        return batteryPercentage > batteryConfigMaxPercentage;
    }

    return batteryVoltage > batteryConfigMax;
}
