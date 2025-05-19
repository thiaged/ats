#pragma once
#include "queue.h"

typedef struct {
    int frequencies[10];  // Frequências das notas (Hz)
    int durations[10];    // Durações das notas (ms)
    int count;            // Número de notas na sequência
  } SoundSequence;

// Definindo a estrutura para armazenar comandos de desenho
struct SoundCommand {
    SoundSequence sequence;
    int channel;
};

class BuzzerManager
{
private:
    int buzzerPin;

    QueueHandle_t soundQueue;

    void funcSoundTask();
    static void soundTask(void *pvParameters);

public:
    BuzzerManager(int pBuzzerPin);
    virtual ~BuzzerManager() = default;
    void Init();
    void PlayInitSound();
    void PlaySyncSound();
    void PlaySyncedSound();
    void PlayErrorSound();
    void PlayClickSound();
};