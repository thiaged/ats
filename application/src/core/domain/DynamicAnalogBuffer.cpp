#include "DynamicAnalogBuffer.h"
#include <numeric>
#include <cmath>

DynamicAnalogBuffer::DynamicAnalogBuffer(
    size_t maxSize,
    uint32_t &pDcOffset
)
    : maxSize(maxSize), currentMean(0.0f), sumDc(0), dcOffset(pDcOffset), sumSquares(0), currentRms(0.0f)
{
}

void DynamicAnalogBuffer::AddReading(int value)
{
    if (readings.size() >= maxSize)
    {
        // Remove o valor mais antigo e ajusta a soma
        sumDc -= readings.front();
        readings.pop_front();
    }

    // Adiciona a nova leitura
    readings.push_back(value);
    sumDc += value;

    // Atualiza a média
    currentMean = sumDc / readings.size();
}

void DynamicAnalogBuffer::AddReadingAc(int value)
{
    bufferPos++;
    readings.push_back(value);

    int acValue = value - dcOffset;
    sumSquares += acValue * acValue;

    // Atualiza o buffer AC e a soma dos quadrados
    if (readings.size() > maxSize) {
        int oldAc = readings.front() - dcOffset;
        sumSquares -= oldAc * oldAc;
        readings.pop_front();
    }

    if (bufferPos >= maxSize)
    {
        readingsDraw = readings;
        bufferPos = 0;

        // Atualiza o RMS AC
        if (!readings.empty()) {
            currentRms = std::sqrt(static_cast<float>(sumSquares) / readings.size());
        }
    }
}

float DynamicAnalogBuffer::GetMean() const
{
    return currentMean;
}

std::deque<unsigned long long> DynamicAnalogBuffer::GetBufferDraw() const
{
    return readingsDraw;
}

float DynamicAnalogBuffer::GetRms() const
{
    return currentRms;
}

uint32_t DynamicAnalogBuffer::GetDcOffset() const
{
    return dcOffset;
}

void DynamicAnalogBuffer::SetDcOffset(uint32_t value)
{
    bool changed = value != dcOffset;
    dcOffset = value;
}
