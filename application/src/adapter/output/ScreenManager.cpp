#include "ScreenManager.h"

void ScreenManager::AddScreen(std::unique_ptr<Screen> screen)
{
    screens.push_back(std::move(screen));
}

void ScreenManager::Render()
{
    if (!screens.empty()) {
        if (currentScreenIndex != homePos) {
            screens[homePos]->redrawHouse = true;
        }
        screens[currentScreenIndex]->Render();
    }
}

void ScreenManager::NextScreen()
{
    previousScreenIndex = currentScreenIndex;
    int next = (currentScreenIndex + 1) % screens.size();

    if(screens[next].get()->isHidden()) {
        next = (next + 1) % screens.size();
    }
    currentScreenIndex = next;
}

void ScreenManager::PreviousScreen()
{
    currentScreenIndex = previousScreenIndex;
}

void ScreenManager::GoToScreen(Screen *screen)
{
    previousScreenIndex = currentScreenIndex;
    for (int i = 0; i < screens.size(); i++) {
        if (screens[i].get() == screen) {
            currentScreenIndex = i;
            break;
        }
    }
    Render();
}
