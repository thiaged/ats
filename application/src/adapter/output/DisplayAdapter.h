#pragma once
#include <TFT_eSPI.h>
#include "ScreenManager.h"

class DisplayAdapter {
private:
    TFT_eSPI& display;
    ScreenManager& screenManager;

public:
    DisplayAdapter(TFT_eSPI& display, ScreenManager& screenManager)
        : display(display), screenManager(screenManager) {}

    void init() {
        display.init();
        display.setRotation(1); // Ajusta a rotação da tela
        display.fillScreen(TFT_BLACK);
    }

    void update() {
        screenManager.Render(); // Renderiza a tela atual
    }
};
