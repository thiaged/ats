#include "ScreenSystemMetrics.h"
#include <Arduino.h>

ScreenSystemMetrics::ScreenSystemMetrics(
    EnergySource &pUtilitySource,
    EnergySource &pSolarSource,
    EnergySource **pActiveSource,
    EnergySource **pDefinedSource,
    EnergySource **pUserDefinedSource,
    BatterySource &pBattery,
    DrawManager &pDrawManager,
    TFT_eSPI &pDisplay,
    LilyGo_Class &pAmoled,
    bool *pUserSourceLocked,
    Preferences &pConfigPreferences
)
    : Screen(pUtilitySource, pSolarSource, pActiveSource, pDefinedSource, pUserDefinedSource, pBattery, pDrawManager, pDisplay,
        pAmoled, pUserSourceLocked, pConfigPreferences)
{
    metricsSprite = new TFT_eSprite(&display);
}

void ScreenSystemMetrics::Render()
{
    Screen::Render();
    if ((millis() - slowScreenAnimMetrics) > 1000) // Update every second
    {
        slowScreenAnimMetrics = millis();
        drawSystemMetrics();
    }
}

void ScreenSystemMetrics::drawSystemMetrics()
{
    if (!metricsSprite) return;

    metricsSprite->createSprite(screenW, screenH);
    metricsSprite->fillSprite(BG_WAVE);
    metricsSprite->setTextColor(TFT_WHITE);

    // Header
    metricsSprite->setTextColor(TFT_CYAN);
    metricsSprite->drawString("System Metrics", 10, 5, 4);

    // Draw different sections
    drawMemoryMetrics();
    drawCPUMetrics();

    drawManager.RequestDraw(screenX, screenY, screenW, screenH, metricsSprite, nullptr, nullptr);
}

void ScreenSystemMetrics::drawMemoryMetrics()
{
    if (!metricsSprite) return;

    // Get memory info
    size_t freeHeap = esp_get_free_heap_size();
    size_t minFreeHeap = esp_get_minimum_free_heap_size();
    size_t largestFreeBlock = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);

    multi_heap_info_t heapInfo;
    heap_caps_get_info(&heapInfo, MALLOC_CAP_INTERNAL);

    size_t totalHeap = heapInfo.total_allocated_bytes + heapInfo.total_free_bytes;
    float heapUsagePercent = totalHeap > 0 ? (100.0 * heapInfo.total_allocated_bytes / totalHeap) : 0.0;

    int yPos = 30;
    int lineHeight = 28;
    char buffer[64];

    metricsSprite->setTextColor(TFT_YELLOW);
    metricsSprite->drawString("Memory:", 10, yPos, 4);
    yPos += lineHeight;

    metricsSprite->setTextColor(TFT_WHITE);

    // Free heap
    snprintf(buffer, sizeof(buffer), "Free: %d KB", (int)(freeHeap / 1024));
    metricsSprite->drawString(buffer, 15, yPos, 4);
    yPos += lineHeight;

    // Min free heap
    snprintf(buffer, sizeof(buffer), "Min: %d KB", (int)(minFreeHeap / 1024));
    metricsSprite->drawString(buffer, 15, yPos, 4);
    yPos += lineHeight;

    // Usage percent
    snprintf(buffer, sizeof(buffer), "Usage: %.1f%%", heapUsagePercent);
    metricsSprite->drawString(buffer, 15, yPos, 4);
    yPos += lineHeight;

}

void ScreenSystemMetrics::drawCPUMetrics()
{
    if (!metricsSprite) return;

    int yPos = 30;
    int lineHeight = 28;
    int xPos = 182; // Right column
    char buffer[64];

    metricsSprite->setTextColor(TFT_YELLOW);
    metricsSprite->drawString("CPU:", xPos, yPos, 4);
    yPos += lineHeight;

    metricsSprite->setTextColor(TFT_WHITE);

    // Uptime
    unsigned long uptimeSeconds = millis() / 1000;
    unsigned long hours = uptimeSeconds / 3600;
    unsigned long minutes = (uptimeSeconds % 3600) / 60;
    unsigned long seconds = uptimeSeconds % 60;
    snprintf(buffer, sizeof(buffer), "Up: %02lu:%02lu:%02lu", hours, minutes, seconds);
    metricsSprite->drawString(buffer, xPos, yPos, 4);
    yPos += lineHeight;

    // CPU Frequency
    snprintf(buffer, sizeof(buffer), "Freq: %d MHz", ESP.getCpuFreqMHz());
    metricsSprite->drawString(buffer, xPos, yPos, 4);
    yPos += lineHeight;

    // Chip info
    esp_chip_info_t chipInfo;
    esp_chip_info(&chipInfo);
    snprintf(buffer, sizeof(buffer), "Cores: %d", (int)chipInfo.cores);
    metricsSprite->drawString(buffer, xPos, yPos, 4);
    yPos += lineHeight;

    // Number of tasks
    snprintf(buffer, sizeof(buffer), "Tasks: %d", (int)uxTaskGetNumberOfTasks());
    metricsSprite->drawString(buffer, xPos, yPos, 4);
}

