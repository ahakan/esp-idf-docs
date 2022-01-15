#include "main.h"

static Main _Main;

extern "C" void app_main(void)
{
    esp_err_t ret = _Main.setup();

    if (ret)
    {
        ESP_LOGE(LOG_TAG, "Not initialized.");
    }

    ESP_ERROR_CHECK(ret);
    ESP_LOGI(LOG_TAG, "Initialized.");

    while (true)
    {
        _Main.loop();
        vTaskDelay(pdSECOND);
    }

    return;
}

esp_err_t Main::setup(void)
{
    ESP_LOGI(LOG_TAG, "Initializing...");

    return ESP_OK;
}

void Main::loop(void)
{
    ESP_LOGI(LOG_TAG, "Main function.");
}