#include "example_neopixel.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    example_neopixel_initialize();

    example_neopixel_set_color(0, 0, 0, 255);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    while (1)
    {
        for (size_t i = 0; i < 10; i++)
        {
            example_neopixel_set_pixel_color(i, 255, 0, 0, 0);

            vTaskDelay(50 / portTICK_PERIOD_MS);
        }

        vTaskDelay(500 / portTICK_PERIOD_MS);

        for (size_t i = 10; i > 0; i--)
        {
            example_neopixel_set_pixel_color(i-1, 0, 0, 255, 0);

            vTaskDelay(50 / portTICK_PERIOD_MS);
        }

        vTaskDelay(500 / portTICK_PERIOD_MS);

        for (size_t i = 0; i < 10; i++)
        {
            example_neopixel_set_pixel_color(i, 0, 255, 0, 0);

            vTaskDelay(50 / portTICK_PERIOD_MS);
        }

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

}
