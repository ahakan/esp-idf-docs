#ifndef EXAMPLE_NEOPIXEL_H
#define EXAMPLE_NEOPIXEL_H

#include <stdio.h>

#include "driver/rmt.h"

#include "esp_log.h"
#include "esp_err.h"

#define NEOPIXEL_PIN 5
#define NEOPIXEL_SIZE 10
// #define RMT_TX_CHANNEL RMT_CHANNEL_0

esp_err_t example_neopixel_initialize();
esp_err_t example_neopixel_deinitialize();

esp_err_t example_neopixel_set_color(uint8_t, uint8_t, uint8_t, uint8_t);
esp_err_t example_neopixel_set_pixel_color(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);


#endif