#pragma once

#include <TFT_eSPI.h>
#include <LilyGo_AMOLED.h>
#include <core/domain/EnergySource.h>

// Definindo a estrutura para armazenar comandos de desenho
struct DrawCommand {
    int x;            // Coordenada X
    int y;            // Coordenada Y
    int w;            // Largura
    int h;            // Altura
    TFT_eSprite* sprite; // Ponteiro para os dados do sprite
    EnergySource* utilitySource;
    EnergySource* solarSource;

};

class DrawManager {
private:
    void funcDrawTask();
    static void drawTask(void *pvParameters);
protected:
    // Fila para armazenar os comandos de desenho
    QueueHandle_t drawQueue;
    LilyGo_Class& amoled;
    bool &inSleepMode;
public:

    DrawManager(
        LilyGo_Class& pAmoled,
        bool &pInSleepMode
    );
    void StartDrawTask();
    void RequestDraw(int x, int y, int w, int h, TFT_eSprite *sprite, EnergySource *utilitySource, EnergySource *solarSource);
    int GetScreenWidth();
    int GetScreenHeight();
    void SetSleepMode(bool sleep);

};
