#include <TFT_eSPI.h>
#include <LilyGo_AMOLED.h>
#include "BuzzerManager.h"

void BuzzerManager::funcSoundTask()
{
    Serial.println("tarefa de som iniciada");
    SoundCommand cmd;
    if (soundQueue == nullptr) {
        Serial.println("fila de som nula");
    }

    while (true) {
        // Aguarda até que um comando de desenho esteja disponível na fila
        if (xQueueReceive(soundQueue, &cmd, portMAX_DELAY) == pdTRUE) {
            // Toca o som
            for (int i = 0; i < cmd.sequence.count; i++) {
                ledcWriteTone(cmd.channel, cmd.sequence.frequencies[i]);
                vTaskDelay(cmd.sequence.durations[i] / portTICK_PERIOD_MS);
              }
              // Desliga o som ao final da sequência
              ledcWriteTone(cmd.channel, 0);
        }
    }
}

void BuzzerManager::soundTask(void *pvParameters)
{
    BuzzerManager *instance = static_cast<BuzzerManager*>(pvParameters);
    instance->funcSoundTask();
}

BuzzerManager::BuzzerManager(int pBuzzerPin)
{
    buzzerPin = pBuzzerPin;
}

void BuzzerManager::Init()
{
    ledcSetup(0, 3000, 8);  // Canal 0, frequência 2000Hz, resolução de 8 bits
    ledcAttachPin(buzzerPin, 0);
    ledcWrite(0, 153);  // Inicializa o buzzer com 50% de duty cycle
    delay(150);
    ledcWriteTone(0, 0);  // Desliga o buzzer

     // Cria a fila com capacidade para 60 comandos de desenho
     soundQueue = xQueueCreate(60, sizeof(SoundCommand));
     Serial.println("fila de som criada");

     // Inicia a tarefa de desenho
     xTaskCreate(soundTask, "SoundTask", 2048, this, 2, NULL);
}

void BuzzerManager::PlayInitSound()
{
    // Toca um efeito sonoro de inicialização (subindo o tom)
    SoundSequence seq = {
        { 500, 750, 1000, 1250, 1500 },
        { 150, 150, 150, 150, 150 },
        5
    };

    SoundCommand cmd = { seq, 0 };
    xQueueSend(soundQueue, &cmd, 0);
}

void BuzzerManager::PlaySyncSound()
{
    SoundSequence seq = {
        { 1500 },
        { 100 },
        1
    };
    SoundCommand cmd = { seq, 0 };
    xQueueSend(soundQueue, &cmd, 0);
}

void BuzzerManager::PlaySyncedSound()
{

    SoundSequence seq = {
        { 1854, 1032, 1000 },
        { 150, 50, 150 },
        4
    };

    SoundCommand cmd = { seq, 0 };
    xQueueSend(soundQueue, &cmd, 0);
}

void BuzzerManager::PlayErrorSound()
{
    // Two-beep error pattern with a short pause between
    SoundSequence seq = {
        { 800, 0, 800 },
        { 200, 80, 300 },
        3
    };
    SoundCommand cmd = { seq, 0 };
    xQueueSend(soundQueue, &cmd, 0);
}

void BuzzerManager::PlayClickSound()
{
    SoundSequence seq = {
        { 1000 },
        { 100 },
        1
    };
    SoundCommand cmd = { seq, 0 };
    xQueueSend(soundQueue, &cmd, 0);
}
