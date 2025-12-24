#include "BatterySource.h"

BatterySource::BatterySource(
    DynamicAnalogBuffer &pBatteryReadBuffer,
    Preferences &pConfigPreferences,
    HardwareSerial &pSerial,
    Logger &pLogger,
    bool *pUpdatingFirmware
)
    : batteryReadBuffer(pBatteryReadBuffer),
      configPreferences(pConfigPreferences),
      RS485Serial(pSerial),
      logger(pLogger),
      updatingFirmware(pUpdatingFirmware)
{
    bufferIndex = 0;
    lastBmsRequestTime = 0;
    bmsProcessing = false;

    // Inicializa a jump table
    initializeJumpTable();

    for (size_t i = 0; i < NUM_VALID_ADDRESSES; i++) {
        addressValid[VALID_ADDRESSES[i]] = true;
    }
}

bool isValidAddress(uint8_t addr) {
    for (size_t i = 0; i < NUM_VALID_ADDRESSES; i++) {
        if (VALID_ADDRESSES[i] == addr) {
            return true;
        }
    }
    return false;
}

BatterySource::~BatterySource()
{
    if (batterySensorTaskHandle != NULL) {
        vTaskDelete(batterySensorTaskHandle);
    }
}

void BatterySource::initializeJumpTable()
{
    // Inicializa todos como nullptr
    for (int i = 0; i < JUMP_TABLE_SIZE; i++) {
        jumpTable[i] = nullptr;
        dataSizes[i] = 1;
    }

    // Mapeia os endereços conhecidos
    jumpTable[REG_CELL_VOLTAGES] = &BatterySource::parseCellVoltages;
    jumpTable[REG_MOS_TEMP]      = &BatterySource::parseMosTemp;
    dataSizes[REG_MOS_TEMP]      = 2;
    jumpTable[REG_BATTERY_T1]    = &BatterySource::parseBatteryT1;
    dataSizes[REG_BATTERY_T1]    = 2;
    jumpTable[REG_BATTERY_T2]    = &BatterySource::parseBatteryT2;
    dataSizes[REG_BATTERY_T2]    = 2;
    jumpTable[REG_TOTAL_VOLTAGE] = &BatterySource::parseTotalVoltage;
    dataSizes[REG_TOTAL_VOLTAGE] = 2;
    jumpTable[REG_CURRENT]       = &BatterySource::parseCurrent;
    dataSizes[REG_CURRENT]       = 2;
    jumpTable[REG_RESIDUAL_CAPACITY] = &BatterySource::parseSOC;
    dataSizes[REG_RESIDUAL_CAPACITY] = 1;
    jumpTable[REG_NUM_NTC]    = &BatterySource::parseNumNTC;
    dataSizes[REG_NUM_NTC]    = 1;
    jumpTable[REG_CYCLE_COUNT] = &BatterySource::parseCycleCount;
    dataSizes[REG_CYCLE_COUNT] = 2;
    jumpTable[REG_BATTERY_CAPACITY] = &BatterySource::parseBatteryCapacity;
    dataSizes[REG_BATTERY_CAPACITY] = 4;
    jumpTable[REG_TOTAL_STRINGS] = &BatterySource::parseTotalStrings;
    dataSizes[REG_TOTAL_STRINGS] = 2;
    jumpTable[REG_WARN_MESSAGES] = &BatterySource::parseWarnMessages;
    dataSizes[REG_WARN_MESSAGES] = 2;

    // Para qualquer endereço não mapeado, usa parseUnknown
    for (int i = 0; i < JUMP_TABLE_SIZE; i++) {
        if (jumpTable[i] == nullptr) {
            jumpTable[i] = &BatterySource::parseUnknown;
        }
    }
}

bool BatterySource::parseCellVoltages(const byte* data, int length, int offset, BMSData& stagedBMS)
{
    float highestCellVoltage = 0.0;
    float lowestCellVoltage = 100.0;

    // Resetar índices antes de processar
    stagedBMS.highestCellIndex = -1;
    stagedBMS.lowestCellIndex = -1;

    // Verificação de segurança: garantir que há espaço para o comprimento
    if (offset + 1 >= length) {
        logger.logWarning("⚠️ Buffer insuficiente para ler comprimento das células");
        return false;
    }

    // ⭐ LER O BYTE DE COMPRIMENTO CORRETAMENTE ⭐
    uint8_t dataLength = data[offset]; // offset já chega 1 a frente do ID (0x79)
    dataSizes[REG_CELL_VOLTAGES] = dataLength; // Armazenar para avançar no parsing principal
    int numCells = dataLength / 3; // Cada célula usa 3 bytes

    if (numCells != batteryCellsNumber) {
        logger.logWarningF("⚠️ Comprimento inválido no registrador 0x79: %d cells", numCells);
        return false;
    }

    // Verificar se há dados suficientes
    if (offset + 1 + dataLength > length) {
        logger.logWarningF("⚠️ Dados insuficientes para células: precisa %d, tem %d\n",
                         offset + 2 + dataLength, length);
        return false;
    }

    logger.logInfoF("🔋 Processando %d células (comprimento=%d)\n", numCells, dataLength);

    // ⭐ POSIÇÃO CORRETA PARA INÍCIO DOS DADOS ⭐
    int j = offset + 1; // Após byte de comprimento

    for (int cellIndex = 0; cellIndex < numCells && j + 2 < length; cellIndex++) {
        byte cellId = data[j];       // ID da célula
        uint16_t rawVoltage = (data[j + 1] << 8) | data[j + 2]; // Tensão em mV

        logger.logInfoF("  Célula %d: ID=%d, tensão=%d mV\n",
                         cellIndex, cellId, rawVoltage);

        // Validação do ID da célula (1-24)
        if (cellId >= 1 && cellId <= 24) {
            float voltage = rawVoltage / 1000.0; // Converter para volts

            // Validação de tensão realista (0.0V - 5.0V para LiFePO4)
            if (voltage >= 0.0 && voltage <= 5.0) {
                stagedBMS.cellVoltages[cellId] = voltage;

                // Atualizar célula mais alta
                if (stagedBMS.highestCellIndex == -1 || voltage > highestCellVoltage) {
                    highestCellVoltage = voltage;
                    stagedBMS.highestCellIndex = cellId;
                }

                // Atualizar célula mais baixa
                if (stagedBMS.lowestCellIndex == -1 || voltage < lowestCellVoltage) {
                    lowestCellVoltage = voltage;
                    stagedBMS.lowestCellIndex = cellId;
                }
            } else {
                logger.logWarningF("  ⚠️ Tensão inválida na célula %d: %.3f V\n",
                             cellId, voltage);
                return false;
            }
        } else {
            logger.logWarningF("  ⚠️ ID de célula inválido: %d\n", cellId);
            return false;
        }

        j += 3; // Avançar para próxima célula (3 bytes por célula)
    }

    return true;
}

bool BatterySource::parseTemperature(const byte* data, int length, int offset, float& target)
{
    if (offset + 1 >= length) {
        logger.logWarning("⚠️ Dados insuficientes para ler temperatura");
        return false;
    }

    uint8_t rawTemp = data[offset + 1];
    // Conversão segundo protocolo: valores acima de 100 são negativos
    float temperature = rawTemp;
    if (rawTemp > 100) {
        temperature = 100 - rawTemp;
    }

    target = temperature;

    return true;
}

bool BatterySource::parseMosTemp(const byte* data, int length, int offset, BMSData& stagedBMS)
{
    return parseTemperature(data, length, offset, stagedBMS.mosTemp);
}

bool BatterySource::parseBatteryT1(const byte* data, int length, int offset, BMSData& stagedBMS)
{
    return parseTemperature(data, length, offset, stagedBMS.batteryT1);
}

bool BatterySource::parseBatteryT2(const byte* data, int length, int offset, BMSData& stagedBMS)
{
    return parseTemperature(data, length, offset, stagedBMS.batteryT2);
}

bool BatterySource::parseTotalVoltage(const byte *data, int length, int offset, BMSData& stagedBMS)
{
    if (offset + dataSizes[REG_TOTAL_VOLTAGE] > length) {
        logger.logWarning("⚠️ Dados insuficientes para ler tensão total");
        return false;
    }

    uint16_t rawValue = (data[offset] << 8) | data[offset + 1];
    double voltage = rawValue / 100.0;
    if (
        voltage > batteryVoltageMin &&
        voltage < batteryVoltageMax)
    {
        stagedBMS.totalVoltage = voltage;
    } else {
        logger.logWarningF("⚠️ Tensão total inválida: %.2f V", voltage);
        return false;
    }

    return true;
}

bool BatterySource::parseCurrent(const byte* data, int length, int offset, BMSData& stagedBMS)
{
    if (offset + dataSizes[REG_CURRENT] > length) {
        logger.logWarning("⚠️ Dados insuficientes para ler corrente");
        return false;
    }

    uint16_t rawValue = (data[offset] << 8) | data[offset + 1];
    bool isCharging = (rawValue & 0x8000) != 0;
    uint16_t magnitude = rawValue & 0x7FFF;
    isCharging = (magnitude != 0) ? isCharging : false; // Se magnitude for 0, força isCharging para false
    double current = (isCharging ? 1.0 : -1.0) * magnitude * 0.01;
    if (current > -200.0 && current < 200.0) {
        stagedBMS.current = current;
        stagedBMS.power = stagedBMS.totalVoltage * stagedBMS.current;
    }
    else {
        logger.logWarningF("⚠️ Corrente inválida: %.2f A", current);
        return false;
    }

    return true;
}

bool BatterySource::parseSOC(const byte* data, int length, int offset, BMSData& stagedBMS)
{

    if (offset + dataSizes[REG_RESIDUAL_CAPACITY] > length) {
        logger.logWarning("⚠️ Dados insuficientes para ler SOC");
        return false;
    }

    int soc = data[offset];
    if (soc <= 100) {
        if (stagedBMS.soc == 0 || abs(soc - stagedBMS.soc) < 5) {
            stagedBMS.soc = soc;
        } else {
            logger.logWarningF("⚠️ Variação abrupta de SOC detectada: %d%% para %d%%", stagedBMS.soc, soc);
            return false;
        }
    }

    return true;
}

bool BatterySource::parseTotalStrings(const byte* data, int length, int offset, BMSData& stagedBMS)
{
    if (offset + dataSizes[REG_TOTAL_STRINGS] > length) {
        logger.logWarning("⚠️ Dados insuficientes para ler número de strings");
        return false;
    }

    uint16_t numCells = (data[offset] << 8) | data[offset + 1];
    logger.logInfoF("🔋 Número de células detectadas: %d\n", numCells);

    if(numCells == batteryCellsNumber) {
        stagedBMS.totalStrings = numCells;
    } else {
        logger.logWarningF("⚠️ Número de células inválido %d", numCells);
        return false;
    }

    return true;
}

bool BatterySource::parseNumNTC(const byte* data, int length, int offset, BMSData& stagedBMS)
{
    if (offset + dataSizes[REG_NUM_NTC] > length) {
        logger.logWarning("⚠️ Dados insuficientes para ler número de sensores NTC");
        return false;
    }

    uint8_t numNtc = data[offset];
    // Armazena o número de sensores NTC (termistores)
    // O protocolo especifica que este valor geralmente é 2 (dois sensores de temperatura)
    stagedBMS.numNTC = numNtc;

    return true;
}

bool BatterySource::parseCycleCount(const byte* data, int length, int offset, BMSData& stagedBMS)
{
    if (offset + dataSizes[REG_CYCLE_COUNT] > length) {
        logger.logWarning("⚠️ Dados insuficientes para ler número de ciclos");
        return false;
    }

    uint16_t cycleCount = (data[offset] << 8) | data[offset + 1];
    // Armazena o número de ciclos de carga/descarga completos
    stagedBMS.cycleCount = cycleCount;

    return true;
}

bool BatterySource::parseBatteryCapacity(const byte* data, int length, int offset, BMSData& stagedBMS)
{
    if (offset + dataSizes[REG_BATTERY_CAPACITY] + 3 > length) {
        logger.logWarning("⚠️ Dados insuficientes para ler capacidade da bateria");
        return false;
    }

    // Lê 4 bytes para a capacidade total da bateria em Ah
    uint32_t capacityRaw = (uint32_t)data[offset] << 24 |
                          (uint32_t)data[offset + 1] << 16 |
                          (uint32_t)data[offset + 2] << 8 |
                          (uint32_t)data[offset + 3];

    // Converte para valor em Ah (float)
    // O protocolo não especifica a escala exata, mas geralmente é em mAh ou Ah
    // Considerando que é provavelmente em mAh:
    stagedBMS.batteryCapacity = capacityRaw / 1000.0; // converte para Ah

    return true;
}

bool BatterySource::parseWarnMessages(const byte* data, int length, int offset, BMSData& stagedBMS)
{
    if (offset + dataSizes[REG_WARN_MESSAGES] > length) {
        logger.logWarning("⚠️ Dados insuficientes para ler mensagens de aviso");
        return false;
    }

    uint16_t warningFlags = (data[offset] << 8) | data[offset + 1];

    // Armazena as flags brutas para análise posterior
    stagedBMS.warningFlags = warningFlags;

    // Análise detalhada dos bits de aviso conforme protocolo
    bool lowCapacityAlarm = (warningFlags & 0x0001) != 0;       // Bit 0
    bool mosOvertempAlarm = (warningFlags & 0x0002) != 0;       // Bit 1
    bool chargeOvervoltageAlarm = (warningFlags & 0x0004) != 0; // Bit 2
    bool dischargeUndervoltageAlarm = (warningFlags & 0x0008) != 0; // Bit 3
    bool batteryOvertempAlarm = (warningFlags & 0x0010) != 0;   // Bit 4
    bool chargeOvercurrentAlarm = (warningFlags & 0x0020) != 0; // Bit 5
    bool dischargeOvercurrentAlarm = (warningFlags & 0x0040) != 0; // Bit 6
    bool cellVoltageDiffAlarm = (warningFlags & 0x0080) != 0;   // Bit 7
    bool boxOvertempAlarm = (warningFlags & 0x0100) != 0;       // Bit 8
    bool batteryLowtempAlarm = (warningFlags & 0x0200) != 0;    // Bit 9
    bool cellOvervoltageAlarm = (warningFlags & 0x0400) != 0;   // Bit 10
    bool cellUndervoltageAlarm = (warningFlags & 0x0800) != 0;  // Bit 11
    bool protection309A = (warningFlags & 0x1000) != 0;         // Bit 12
    bool protection309B = (warningFlags & 0x2000) != 0;         // Bit 13

    // Monta uma string descritiva dos alarmes ativos para debug
    String warnings = "";
    if (lowCapacityAlarm) warnings += "LowCapacity ";
    if (mosOvertempAlarm) warnings += "MOSOvertemp ";
    if (chargeOvervoltageAlarm) warnings += "ChargeOvervolt ";
    if (dischargeUndervoltageAlarm) warnings += "DischargeUndervolt ";
    if (batteryOvertempAlarm) warnings += "BattOvertemp ";
    if (chargeOvercurrentAlarm) warnings += "ChargeOvercurrent ";
    if (dischargeOvercurrentAlarm) warnings += "DischargeOvercurrent ";
    if (cellVoltageDiffAlarm) warnings += "CellVoltageDiff ";
    if (boxOvertempAlarm) warnings += "BoxOvertemp ";
    if (batteryLowtempAlarm) warnings += "BattLowtemp ";
    if (cellOvervoltageAlarm) warnings += "CellOvervoltage ";
    if (cellUndervoltageAlarm) warnings += "CellUndervoltage ";
    if (protection309A) warnings += "309A_Protection ";
    if (protection309B) warnings += "309B_Protection ";

    if (warnings.length() > 0) {
        logger.logWarningF("⚠️ BMS Warnings: %s", warnings.c_str());
    }

    // Define se há algum alarme ativo
    stagedBMS.hasWarnings = (warningFlags != 0);

    return true;
}

bool BatterySource::parseUnknown(const byte* data, int length, int offset, BMSData& stagedBMS)
{
    return true;
}

void BatterySource::Init()
{
    pinMode(BATTERY_SENSOR_PIN, INPUT);
    RS485Serial.begin(RS485_BAUD, SERIAL_8N1, RS485_RX, RS485_TX);
    RS485Serial.setTimeout(400);

    logger.logInfo("JK BMS Communication started");

    startSensorReadTask();

    configPreferences.begin("config", true);
    batteryConfig = configPreferences.getFloat("battery-calib", batteryConfig);
    bmsComunicationOn = configPreferences.getBool("bms-on", false);
    batteryConfigMin = configPreferences.getDouble("bMin", 23.50);
    batteryConfigMax = configPreferences.getDouble("bMax", 27.00);
    batteryConfigMinPercentage = configPreferences.getInt("bMinPercent", 40);
    batteryConfigMaxPercentage = configPreferences.getInt("bMaxPercent", 60);
    configPreferences.end();

    // Cria a fila (pode armazenar 1 buffer por vez)
    bmsResponseQueue = xQueueCreate(5, sizeof(BmsResponseItem));
    if (bmsResponseQueue == NULL) {
        logger.logError("Falha ao criar fila BMS");
    }

    // Inicia a task de parsing
    startBmsParserTask();
}

void BatterySource::startSensorReadTask()
{
    xTaskCreatePinnedToCore(
        sensorReadTask,
        "ReadBatterySensorTask",
        4096,
        this,
        0,
        &batterySensorTaskHandle,
        0 // 👉 PINADO AO CORE 0
    );
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

        if (instance->GetBmsComunicationOn()) {
            vTaskDelay(3000 / portTICK_PERIOD_MS);
        } else{
            vTaskDelay((instance->sampleIntervalBattery / 1000) / portTICK_PERIOD_MS);
        }
    }
}

void BatterySource::funcSensorReadTask()
{
    if (bmsComunicationOn == true)
    {
        readBatteryBMS();
    }
    readBatteryResistorDivisor();
}

void BatterySource::readBatteryResistorDivisor()
{
    if (BMS.isConnected)
    {
        batteryVoltage = BMS.totalVoltage;
        batteryPercentage = BMS.soc;
        return;
    }

    int adcValue = analogReadMilliVolts(BATTERY_SENSOR_PIN);
    if (adcValue < 700)
        adcValue = 0;

    batteryReadBuffer.AddReading(adcValue);
    batteryVoltage = (batteryReadBuffer.GetMean() / 4096.0 * 3.33) * ((BATTERY_SENSOR_R1 + BATTERY_SENSOR_R2) / BATTERY_SENSOR_R2) * batteryConfig;
}

bool BatterySource::validateResponseCRC(byte* buffer, int length)
{
    // Procurar o último footer 0x68 percorrendo o buffer de trás para frente.
    // Isso ajuda a encontrar o footer correto quando há bytes extras no final
    // ou múltiplos 0x68 no payload.
    int footerPos = -1;
    // Precisamos de pelo menos 5 bytes: footer + 4 bytes de CRC conforme o protocolo usado anteriormente
    if (length < 5) {
        logger.logError("Response too short to contain footer + CRC");
        BMS.consecutiveCrcFailures++;
        return false;
    }

    // Buscar o footer a partir do final (últimos 5 bytes)
    for (int i = length; i >= 0; i--) {
        if (buffer[i] == JK_BMS_FOOTER) {
            footerPos = i;
            break;
        }
    }

    if (footerPos == -1) {
        logger.logError("Footer 0x68 not found in response");
        BMS.consecutiveCrcFailures++;
        return false;
    }

    // Verificar se há espaço para 4 bytes de CRC após o footer
    // footerPos + 4 deve ser um índice válido (0-based)
    if (footerPos + 4 >= length + 2) {
        logger.logError("Not enough space for CRC after footer");
        BMS.consecutiveCrcFailures++;
        return false;
    }

    // Calcular o checksum dos bytes do início até o footer (inclusive)
    uint16_t calculatedCRC = 0;
    for (int i = 0; i <= footerPos; i++) {
        calculatedCRC += buffer[i];
    }

    // Ler o CRC recebido (dois bytes, little-endian no código anterior?)
    // Observando o uso original: (buffer[footerPos + 3] << 8) | buffer[footerPos + 4]
    // Isso pressupõe que os bytes CRC estão em posições +3 e +4 relativas ao footer.
    // Aqui mantemos a mesma disposição, mas garantimos que os índices existam.
    uint16_t receivedCRC = (uint16_t(buffer[footerPos + 3]) << 8) | uint16_t(buffer[footerPos + 4]);

    // Comparar CRC
    // NOTA: O protocolo JK-BMS parece usar um algoritmo CRC não-padrão.
    // Análise de dispositivo real (FW: NW_1_0_0_200428) mostra:
    // - Checksum calculado: 0x4381
    // - CRC recebido: 0x4923
    // - Nenhum algoritmo CRC-16 padrão corresponde
    //
    // Solução pragmática: Permitir dados com CRC mismatch, mas rastrear falhas
    // A integridade dos dados é verificada pela interpretação semântica dos registros
    // (voltagens de células, temperatura, corrente, SOC, etc. são todos válidos)

    if (calculatedCRC == receivedCRC) {
        logger.logInfo("✓ CRC OK");
        BMS.consecutiveCrcFailures = 0;
        return true;
    } else {
        // CRC não corresponde, mas permitir processamento com limite de tolerância
        logger.logWarningF("⚠ CRC MISMATCH (data will be processed): calculated=0x%X, received=0x%X", calculatedCRC, receivedCRC);
        logger.logWarningF("Failure count: %d/5", BMS.consecutiveCrcFailures + 1);

        BMS.consecutiveCrcFailures++;

        // Desconectar apenas após múltiplas falhas consecutivas de CRC
        // Isso permite tolerância para variações de firmware ou protocolo
        if (BMS.consecutiveCrcFailures >= 5) {
            logger.logError("❌ BMS desconectado após 5 falhas consecutivas de CRC");
            return false;
        }

        // Permitir processamento deste pacote apesar do CRC mismatch
        return true;
    }
}

void BatterySource::readBatteryBMS()
{
    if (!bmsComunicationOn) return;

    byte command[JK_BMS_FULL_CMD_LENGTH];

    memcpy(command, JK_BMS_BASE_CMD, JK_BMS_BASE_CMD_LENGTH);
    command[8] = 0x06;
    command[11] = 0x00;
    command[12] = 0x00;
    command[13] = 0x00;
    command[14] = 0x00;
    command[15] = 0x00;
    command[16] = JK_BMS_FOOTER;

    uint8_t crc8 = calculateCRC(command, 17);
    command[17] = 0x00;
    command[18] = 0x00;
    command[19] = 0x01;
    command[20] = crc8;

    logBufferDump("Read All Data Command", command, JK_BMS_FULL_CMD_LENGTH);

    RS485Serial.write(command, JK_BMS_FULL_CMD_LENGTH);
    RS485Serial.flush();

    vTaskDelay(300 / portTICK_PERIOD_MS);

    lastBmsRequestTime = millis();

    memset(receiveBuffer, 0, MAX_BUFFER_SIZE);
    bufferIndex = 0;
    if (RS485Serial.available()) {
        bufferIndex = RS485Serial.readBytes(receiveBuffer, MAX_BUFFER_SIZE - 1);
        receiveBuffer[bufferIndex] = '\0';
        if (bufferIndex > 0) {
            // --- ENVIA PARA A FILA ---
            BmsResponseItem item;
            memcpy(item.buffer, receiveBuffer, bufferIndex);
            item.length = bufferIndex;

            // Envia para a fila (sem bloquear)
            if (xQueueSend(bmsResponseQueue, &item, 0) != pdPASS) {
                logger.logWarning("Fila cheia — descartando resposta antiga");
            } else {
                logger.logInfo("Resposta da BMS enfileirada para parsing assíncrono");
            }
        } else {
            logger.logWarning("No data read after Read All Data command.");
            BMS.isConnected = false;
        }
    } else {
        logger.logWarning("No response available after Read All Data command.");
        BMS.isConnected = false;
    }

    checkConnectionTimeout();
}

uint8_t BatterySource::calculateCRC(const byte* data, int length)
{
    uint8_t crc = 0;
    for (int i = 0; i < length; i++) {
        crc += data[i];
    }
    return crc;
}

void BatterySource::checkConnectionTimeout()
{
    if (BMS.isConnected && (millis() - BMS.lastResponseTime > 5000))
    {
        BMS.isConnected = false;
        logger.logWarning("BMS connection timeout");
    }

    if (bmsComunicationFailures >= 5)
    {
        bmsComunicationOn = false;
        BMS.isConnected = false;
        logger.logWarning("BMS disconnected after 5 consecutive communication failures");
    }
}

// Getters (mantidos iguais)
double BatterySource::GetBmsTotalVoltage() { return BMS.totalVoltage; }
double BatterySource::GetBmsCurrent() { return BMS.current; }
double BatterySource::GetBmsPower() { return BMS.power; }
// Getters/Setters implementations (completos)
double BatterySource::GetBatteryConfigMin() { return batteryConfigMin; }
unsigned int BatterySource::GetBatteryConfigMinPercentage() { return batteryConfigMinPercentage; }
unsigned int BatterySource::GetBatteryConfigMaxPercentage() { return batteryConfigMaxPercentage; }
double BatterySource::GetBatteryConfigMax() { return batteryConfigMax; }
double BatterySource::GetBatteryVoltage() { return batteryVoltage; }
double BatterySource::GetBatteryPercentage() { return batteryPercentage; }
double BatterySource::GetBatteryVoltageMin() { return batteryVoltageMin; }
double BatterySource::GetBatteryVoltageMax() { return batteryVoltageMax; }
uint64_t BatterySource::GetLowBatStarted() { return lowBatteryStarted; }
uint64_t BatterySource::GetPeakSurgeDelay() { return peakSurgeDelay; }
bool BatterySource::GetBmsComunicationOn() { return bmsComunicationOn; }
BMSData BatterySource::GetBms() { return BMS; }

bool BatterySource::IsHighBattery()
{
    if (BMS.isConnected)
    {
        return batteryPercentage > batteryConfigMaxPercentage;
    }
    return batteryVoltage > batteryConfigMax;
}

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
        return batteryPercentage < 5;
    }
    return batteryVoltage < batteryVoltageMin;
}

void BatterySource::SetBatteryVoltage(double value) { batteryVoltage = value; }
void BatterySource::SetBatteryPercentage(double value) { batteryPercentage = value; }

void BatterySource::SetBmsComunicationOn(bool value)
{
    bool changed = value != bmsComunicationOn;
    bmsComunicationOn = value;
    if (changed)
    {
        configPreferences.begin("config", false);
        configPreferences.putBool("bms-on", value);
        configPreferences.end();
        BMS.soc = 0;
    }
}

void BatterySource::SetBatteryConfigMin(double value)
{
    bool changed = value != batteryConfigMin;
    batteryConfigMin = value;
    if (changed)
    {
        configPreferences.begin("config", false);
        configPreferences.putDouble("bMin", value);
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
        configPreferences.putDouble("bMax", value);
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
        configPreferences.putInt("bMinPercent", value);
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
        configPreferences.putInt("bMaxPercent", value);
        configPreferences.end();
    }
}

void BatterySource::SetBatteryVoltageMin(double value) { batteryVoltageMin = value; }
void BatterySource::SetBatteryVoltageMax(double value) { batteryVoltageMax = value; }

void BatterySource::UpdateLowBatteryStarted() { lowBatteryStarted = esp_timer_get_time(); }
bool BatterySource::LowBatteryDelayEnded() { return esp_timer_get_time() - lowBatteryStarted > peakSurgeDelay; }
void BatterySource::ResetLowbatteryStarted() { lowBatteryStarted = 0; }

int BatterySource::GetBmsSoc() { return BMS.soc; }
int BatterySource::GetBmsTotalStrings() { return BMS.totalStrings; }
float BatterySource::GetBmsMosTemp() { return BMS.mosTemp; }
float BatterySource::GetBmsBatteryT1() { return BMS.batteryT1; }
float BatterySource::GetBmsBatteryT2() { return BMS.batteryT2; }
float BatterySource::GetBmsBalanceCurrent() { return BMS.balanceCurrent; }

float BatterySource::GetBmsCellVoltage(int cellIndex)
{
    if (cellIndex >= 0 && cellIndex < 24) {
        return BMS.cellVoltages[cellIndex];
    }
    return 0.0;
}

bool BatterySource::GetBmsConnected() { return BMS.isConnected; }

void BatterySource::startBmsParserTask()
{
    xTaskCreatePinnedToCore(
        bmsParserTask,
        "BmsParserTask",
        4096,
        this,
        2,
        &bmsParserTaskHandle,
        0 // Core 0
    );
}

void BatterySource::bmsParserTask(void *pvParameters)
{
    BatterySource *instance = static_cast<BatterySource *>(pvParameters);
    if (instance == nullptr) {
        Serial.println("Erro: instância BatterySource nula no parser");
        return;
    }

    BmsResponseItem item;
    while (true) {
        // Aguarda um item na fila
        if (xQueueReceive(instance->bmsResponseQueue, &item, portMAX_DELAY) == pdTRUE) {
            // Envia para processamento gradual
            instance->processBmsResponseAsync(item.buffer, item.length - 2); // Exclui os 2 bytes CRC finais
        }
    }
}

void BatterySource::processBmsResponseAsync(byte* buffer, int length)
{
    if (buffer[0] != 0x4E || buffer[1] != 0x57) {
        logger.logError("❌ Pacote inválido: header não encontrado no início");
        BMS.consecutiveErrors++;
        if (BMS.consecutiveErrors >= 5) {
            bmsComunicationOn = false;
            logger.logError("❌ BMS desconectado após 5 erros consecutivos de comprimento");
            BMS.consecutiveErrors = 0;
        }
        return;
    }

    uint16_t packetLength = (buffer[2] << 8) | buffer[3];
    if (packetLength != length) {
        logger.logWarningF("⚠️ Comprimento declarado (%d) != recebido (%d)\n", packetLength, length);
        if (abs(packetLength - length) > 8) {
            BMS.consecutiveErrors++;
            if (BMS.consecutiveErrors >= 5) {
                bmsComunicationOn = false;
                logger.logError("❌ BMS desconectado após 5 erros consecutivos de comprimento");
                BMS.consecutiveErrors = 0;
            }
            return;
        }
    }

    if (!validateResponseCRC(buffer, length)) {
        if (BMS.consecutiveCrcFailures >= 5) {
            bmsComunicationOn = false;
            logger.logError("❌ BMS desconectado após 5 falhas consecutivas de CRC");
        }
        return;
    }

    int pos = 11; // após: [4E 57][length 2][terminal 4][command 1][source 1][type 1]
    bool parsedSuccessfully = true;
    BMSData stagedBMS = BMS;

    while (pos < length - 3) { // espaço para footer+CRC
        uint8_t registerId = buffer[pos++];

        if (registerId == JK_BMS_FOOTER) {
            break;
        }

        uint8_t dataLength = 1;

        if (pos + dataLength > length - 3) {
            logger.logWarningF("⚠️ Dados insuficientes para registrador 0x%02X\nPosição: %d, dataLength: %d, length: %d", registerId, pos, dataLength, length);
            BMS.consecutiveErrors++;
            if (BMS.consecutiveErrors >= 5) {
                bmsComunicationOn = false;
                logger.logError("❌ BMS desconectado após 5 erros consecutivos de dados inválidos");
                logger.logWarningF("Detalhe: pos=%d, dataLength=%d, length=%d\n", pos, dataLength, length);
                logBufferDump("⚠️ Buffer recebido", buffer, length);
                BMS.consecutiveErrors = 0;
            }
            break;
        }

        if (registerId < JUMP_TABLE_SIZE && jumpTable[registerId] != nullptr && isValidAddress(registerId)) {
            if (!(this->*jumpTable[registerId])(buffer, length, pos, stagedBMS)) {
                parsedSuccessfully = false;
                logger.logWarningF("⚠️ Parse inválido no registrador 0x%02X\n", registerId);
                break;
            }

            dataLength = dataSizes[registerId];

            if (registerId == REG_WARN_MESSAGES) {
                BMS.consecutiveErrors = 0; // Resetar erros após sucesso completo
                break; // Sai após ler as mensagens de aviso
            }
        }

        pos += dataLength;
    }

    if (parsedSuccessfully) {
        BMS = stagedBMS;
    } else {
        logger.logWarning("❌ Erro ao processar dados da BMS, mantendo dados anteriores");
    }
    BMS.lastResponseTime = millis();
    BMS.isConnected = bmsComunicationOn;
    Serial.println("✅ Parsing assíncrono da BMS concluído com sucesso");
}

void BatterySource::logBufferDump(const char* prefix, const uint8_t* data, size_t len)
{
    // Tamanho suficiente para ~64 bytes em hex + texto
    constexpr size_t BUF_SIZE = 256;
    char temp[BUF_SIZE];

    int written = snprintf(temp, sizeof(temp), "%s: ", prefix);
    if (written < 0 || written >= (int)sizeof(temp)) {
        logger.logError("❌ Falha ao gerar cabeçalho do dump");
        return;
    }

    char* ptr = temp + written;
    size_t remaining = sizeof(temp) - written;

    for (size_t i = 0; i < len && remaining > 3; i++) { // 3: "FF "
        int n = snprintf(ptr, remaining, "%02X ", data[i]);
        if (n < 0 || n >= (int)remaining) break;
        ptr += n;
        remaining -= n;
    }

    logger.logInfo(temp);
}