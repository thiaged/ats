#pragma once
#include <vector>
#include <memory>
#include "Screen.h"

class ScreenManager {
private:
    std::vector<std::unique_ptr<Screen>> screens;
    uint8_t currentScreenIndex = 0;
    uint8_t previousScreenIndex = 0;
    uint8_t homePos = 0;

public:
    void AddScreen(std::unique_ptr<Screen> screen);
    void Render();
    void NextScreen();
    void PreviousScreen();
    void GoToScreen(Screen* screen);
};
