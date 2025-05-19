#include "DrawManager.h"

DrawManager::DrawManager(
    LilyGo_Class &pAmoled,
    bool &pInSleepMode
)
    : amoled(pAmoled), inSleepMode(pInSleepMode)
{
}

void DrawManager::RequestDraw(int x, int y, int w, int h, TFT_eSprite *sprite, EnergySource *utilitySource, EnergySource *solarSource)
{
    DrawCommand cmd = { x, y, w, h, sprite, utilitySource, solarSource };

    if (! inSleepMode) {
        if (xQueueSend(drawQueue, &cmd, 0) != pdTRUE) {
            Serial.println("fila de desenho cheia");
            sprite->deleteSprite();
        }
    }

}

int DrawManager::GetScreenWidth()
{
    return amoled.width();
}

int DrawManager::GetScreenHeight()
{
    return amoled.height();
}

void DrawManager::funcDrawTask()
{
    Serial.println("tarefa de desenho iniciada");
    DrawCommand cmd;
    if (drawQueue == nullptr) {
        Serial.println("fila de desenho nula");
    }

    while (true) {
        // Aguarda até que um comando de desenho esteja disponível na fila
        if (xQueueReceive(drawQueue, &cmd, portMAX_DELAY) == pdTRUE) {
            // Desenha o sprite na tela
            if (cmd.sprite->getPointer() == nullptr) {
                // Serial.println("sprite nulo: ");
                continue;
            }
            amoled.pushColors(cmd.x, cmd.y, cmd.w, cmd.h, (uint16_t *)cmd.sprite->getPointer());

            cmd.sprite->deleteSprite();
        }

        vTaskDelay(10 / portTICK_PERIOD_MS); // Delay para evitar uso excessivo da CPU
    }
}

void DrawManager::drawTask(void *pvParameters)
{
    DrawManager* instance = static_cast<DrawManager*>(pvParameters);
    instance->funcDrawTask();
}

void DrawManager::StartDrawTask()
{
    // Cria a fila com capacidade para 60 comandos de desenho
    drawQueue = xQueueCreate(60, sizeof(DrawCommand));
    Serial.println("fila de desenho criada");

    // Inicia a tarefa de desenho
    xTaskCreate(drawTask, "DrawTask", 8100, this, 2, NULL);
}
