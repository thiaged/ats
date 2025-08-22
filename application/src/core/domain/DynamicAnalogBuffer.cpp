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

        // Improved RMS calculation with better smoothing
        int readingsSize = readings.size();
        if (readingsSize > 0) {
            float newRms = std::sqrt(sumSquares / readingsSize);
            
            // Improved smoothing algorithm
            if (currentRms == 0) {
                currentRms = newRms;
            } else {
                // Adaptive smoothing based on RMS stability
                float rmsChange = abs(newRms - currentRms) / currentRms;
                
                float smoothingFactor;
                if (rmsChange < 0.02f) { // Very stable RMS
                    smoothingFactor = 0.85f; // Heavy smoothing
                } else if (rmsChange < 0.08f) { // Moderately stable
                    smoothingFactor = 0.70f; // Medium smoothing
                } else {
                    smoothingFactor = 0.50f; // Light smoothing for unstable values
                }
                
                currentRms = smoothingFactor * currentRms + (1.0f - smoothingFactor) * newRms;
            }
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
