#pragma once
#include <deque>
#include <cmath>
#include <iostream>
#include <Preferences.h>

class DynamicAnalogBuffer
{
private:
    std::deque<unsigned long long> readings;
    std::deque<unsigned long long> readingsDraw;
    uint32_t &dcOffset;       // Offset DC
    size_t maxSize;           // Tamanho máximo do buffer
    volatile float currentMean;        // Média atual calculada dinamicamente
    volatile uint64_t sumDc;           // Soma dos valores no buffer
    volatile uint64_t sumSquares;      // Soma dos quadrados dos valores no buffer
    volatile double currentRms;        // RMS atual calculado dinamicamente
    volatile int bufferPos = 0;

public:
    DynamicAnalogBuffer(size_t maxSize, uint32_t &pDcOffset);

    // Adiciona uma nova leitura ao buffer e atualiza a média
    void AddReading(int value);
    void AddReadingAc(int value);

    // Retorna a média atual
    float GetMean() const;

    float GetRms() const;

    uint32_t GetDcOffset() const;
    void SetDcOffset(uint32_t value);

    // Retorna o buffer atual
    std::deque<unsigned long long> GetBufferDraw() const;

};