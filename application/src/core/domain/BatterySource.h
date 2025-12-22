#pragma once
#include <Arduino.h>
#include <esp_timer.h>
#include <HardwareSerial.h>
#include "DynamicAnalogBuffer.h"
#include <Preferences.h>
#include "queue.h"
#include <adapter/output/Logger.h>

#define BATTERY_SENSOR_R1 100000.0
#define BATTERY_SENSOR_R2 10000.0
#define BATTERY_SENSOR_PIN 3

const int MAX_BUFFER_SIZE = 512;

struct BmsResponseItem {
    byte buffer[MAX_BUFFER_SIZE];
    int length;
};

// Endereços válidos da BMS JK
const uint8_t VALID_ADDRESSES[] = {
    0x79, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x89,
    0x8A, 0x8B, 0x8C, 0x8E, 0x8F, 0x90, 0x91, 0x93, 0x94, 0x95,
    0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9F, 0xA0, 0xA1, 0xA2,
    0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC,
    0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
    0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0
};

const size_t NUM_VALID_ADDRESSES = sizeof(VALID_ADDRESSES) / sizeof(VALID_ADDRESSES[0]);

#define RS485_RX 44
#define RS485_TX 43
#define RS485_BAUD 115200

// JK BMS Protocol constants
#define JK_BMS_HEADER_1 0x4E
#define JK_BMS_HEADER_2 0x57
#define JK_BMS_FOOTER 0x68

// Register addresses for JK BMS
#define REG_CELL_VOLTAGES 0x79
#define REG_MOS_TEMP 0x80
#define REG_BATTERY_T1 0x81
#define REG_BATTERY_T2 0x82
#define REG_TOTAL_VOLTAGE 0x83
#define REG_CURRENT 0x84
#define REG_RESIDUAL_CAPACITY 0x85
#define REG_NUM_NTC 0x86
#define REG_CYCLE_COUNT 0x87
#define REG_BATTERY_CAPACITY 0x89
#define REG_TOTAL_STRINGS 0x8A
#define REG_WARN_MESSAGES 0x8B

// Base command structure
const byte JK_BMS_BASE_CMD[] = {0x4E, 0x57, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00};
const int JK_BMS_BASE_CMD_LENGTH = 11;
const int JK_BMS_FULL_CMD_LENGTH = 21; // Atualizado para 21 bytes (4 de CRC)

// Estrutura para armazenar os dados da BMS
struct BMSData {
  double totalVoltage = 0.0;
  double current = 0.0;
  double power = 0.0;
  float mosTemp = 0.0;
  float batteryT1 = 0.0;
  float batteryT2 = 0.0;
  int soc = 0;
  int totalStrings = 0;
  uint8_t numNTC = 0;
  uint16_t cycleCount = 0;
  float batteryCapacity = 0.0;
  uint16_t warningFlags = 0;
  bool hasWarnings = false;
  float cellVoltages[24] = {0}; // Suporte para até 24 células
  int highestCellIndex = -1;
  int lowestCellIndex = -1;
  float balanceCurrent = 0.0;
  bool isConnected = false;
  unsigned long lastResponseTime = 0;
  unsigned int consecutiveCrcFailures = 0;
  unsigned int consecutiveErrors = 0;
};

class BatterySource
{
private:
    Preferences &configPreferences;
    DynamicAnalogBuffer &batteryReadBuffer;
    float batteryConfig = 1.634;
    Logger &logger;
    u8_t batteryCellsNumber = 8;
    double batteryConfigMin = 23.50;
    unsigned int batteryConfigMinPercentage = 40;
    unsigned int batteryConfigMaxPercentage = 60;
    double batteryConfigMax = 27.00;
    double batteryVoltage = 0.00;
    unsigned int batteryPercentage = 0;
    double batteryVoltageMin = 20.00;
    double batteryVoltageMax = 29.00;
    const uint64_t peakSurgeDelay = 10000000;
    uint64_t lowBatteryStarted = 0;
    const int sampleIntervalBattery = 30000;
    bool bmsComunicationOn = false;
    unsigned int bmsComunicationFailures = 0;
    HardwareSerial& RS485Serial;
    bool *updatingFirmware;
    BMSData BMS;
    TaskHandle_t batterySensorTaskHandle = NULL;
    int bufferIndex = 0;
    byte receiveBuffer[MAX_BUFFER_SIZE];
    unsigned long lastBmsRequestTime = 0;

    // --- FILA PARA PARSING ASSÍNCRONO ---
    QueueHandle_t bmsResponseQueue = NULL;
    TaskHandle_t bmsParserTaskHandle = NULL;

    bool addressValid[256] = {false};

    void startBmsParserTask();
    static void bmsParserTask(void *pvParameters);
    void processNextBmsRegister();

    // Nova flag para evitar sobreposição de processamento
    bool bmsProcessing = false;

    // --- JUMP TABLE ---
    using ParserFunc = void (BatterySource::*)(const byte* data, int length, int offset);
    static constexpr int JUMP_TABLE_SIZE = 256;
    ParserFunc jumpTable[JUMP_TABLE_SIZE] = {nullptr};
    uint8_t dataSizes[JUMP_TABLE_SIZE] = {0};

    // Funções de parsing específicas
    void parseCellVoltages(const byte* data, int length, int offset);
    void parseTemperature(const byte* data, int length, int offset, float& target);
    void parseMosTemp(const byte* data, int length, int offset);
    void parseBatteryT1(const byte* data, int length, int offset);
    void parseBatteryT2(const byte* data, int length, int offset);
    void parseTotalVoltage(const byte* data, int length, int offset);
    void parseCurrent(const byte* data, int length, int offset);
    void parseSOC(const byte* data, int length, int offset);
    void parseTotalStrings(const byte* data, int length, int offset);
    void parseNumNTC(const byte* data, int length, int offset);
    void parseCycleCount(const byte* data, int length, int offset);
    void parseBatteryCapacity(const byte* data, int length, int offset);
    void parseWarnMessages(const byte* data, int length, int offset);
    void parseUnknown(const byte* data, int length, int offset);

    void initializeJumpTable();

    // --- Métodos existentes ---
    void funcSensorReadTask();
    static void sensorReadTask(void *pvParameters);
    void startSensorReadTask();
    void readBatteryResistorDivisor();
    void readBatteryBMS();
    void checkConnectionTimeout();
    uint8_t calculateCRC(const byte* data, int length);
    bool validateResponseCRC(byte* buffer, int length);
    void processAllData(byte* buffer, int length);
    void processBmsResponseAsync(byte* buffer, int length);

    void logBufferDump(const char* prefix, const uint8_t* data, size_t len);

public:
    BatterySource(
        DynamicAnalogBuffer &pBatteryReadBuffer,
        Preferences &pConfigPreferences,
        HardwareSerial &pSerial,
        Logger &pLogger,
        bool *pUpdatingFirmware
    );
    ~BatterySource();
    void Init();

    // Getters/Setters (mantidos iguais)
    double GetBatteryConfigMin();
    unsigned int GetBatteryConfigMinPercentage();
    unsigned int GetBatteryConfigMaxPercentage();
    double GetBatteryConfigMax();
    double GetBatteryVoltage();
    double GetBatteryPercentage();
    double GetBatteryVoltageMin();
    double GetBatteryVoltageMax();
    uint64_t GetLowBatStarted();
    uint64_t GetPeakSurgeDelay();
    bool GetBmsComunicationOn();
    BMSData GetBms();
    bool IsHighBattery();
    bool IsLowBattery();
    bool IsEmpty();
    void SetBatteryVoltage(double value);
    void SetBatteryPercentage(double value);
    void SetBmsComunicationOn(bool value);
    void SetBatteryConfigMin(double value);
    void SetBatteryConfigMax(double value);
    void SetBatteryConfigMinPercentage(int value);
    void SetBatteryConfigMaxPercentage(int value);
    void SetBatteryVoltageMin(double value);
    void SetBatteryVoltageMax(double value);
    void UpdateLowBatteryStarted();
    bool LowBatteryDelayEnded();
    void ResetLowbatteryStarted();

    // JK BMS data getters
    double GetBmsTotalVoltage();
    double GetBmsCurrent();
    double GetBmsPower();
    int GetBmsSoc();
    int GetBmsTotalStrings();
    float GetBmsMosTemp();
    float GetBmsBatteryT1();
    float GetBmsBatteryT2();
    float GetBmsBalanceCurrent();
    float GetBmsCellVoltage(int cellIndex);
    bool GetBmsConnected();
};