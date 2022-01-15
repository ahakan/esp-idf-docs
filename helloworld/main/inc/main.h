#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define LOG_LEVEL_LOCAL ESP_LOG_VERBOSE
#define LOG_TAG "MAIN"

#define pdSECOND pdMS_TO_TICKS(1000)

class Main final
{
    public:
        esp_err_t setup(void);
        void loop(void);
};
