#include "example_neopixel.h"


#define WS2812_T0H_NS (300)
#define WS2812_T0L_NS (900)
#define WS2812_T1H_NS (900)
#define WS2812_T1L_NS (300)

static uint32_t t0h_ticks = 0;
static uint32_t t1h_ticks = 0;
static uint32_t t0l_ticks = 0;
static uint32_t t1l_ticks = 0;

rmt_config_t config;
rmt_channel_t xChannel = RMT_CHANNEL_0;

uint8_t *pixels;

static void IRAM_ATTR ws2812_rmt_adapter(const void *src, rmt_item32_t *dest, size_t src_size,
        size_t wanted_num, size_t *translated_size, size_t *item_num)
{
    if (src == NULL || dest == NULL) 
    {
        *translated_size = 0;
        *item_num = 0;
        return;
    }
    const rmt_item32_t bit0 = {{{ t0h_ticks, 1, t0l_ticks, 0 }}}; //Logical 0
    const rmt_item32_t bit1 = {{{ t1h_ticks, 1, t1l_ticks, 0 }}}; //Logical 1

    size_t size = 0;
    size_t num = 0;

    uint8_t *psrc = (uint8_t *)src;
    rmt_item32_t *pdest = dest;

    while (size < src_size && num < wanted_num) 
    {
        for (int i = 0; i < 8; i++) 
        {
            // MSB first
            if (*psrc & (1 << (7 - i))) 
            {
                pdest->val =  bit1.val;
            } 
            else 
            {
                pdest->val =  bit0.val;
            }
            num++;
            pdest++;
        }
        size++;
        psrc++;
    }
    *translated_size = size;
    *item_num = num;
}

esp_err_t example_neopixel_initialize()
{
    // config = RMT_DEFAULT_CONFIG_TX(NEOPIXEL_PIN, RMT_TX_CHANNEL);

    rmt_config_t config = {
        .rmt_mode = RMT_MODE_TX,
        .channel = xChannel,
        .gpio_num = NEOPIXEL_PIN,
        .clk_div = 2,
        .mem_block_num = 3,
        .tx_config = {
            .carrier_freq_hz = 38000,
            .carrier_level = RMT_CARRIER_LEVEL_HIGH,
            .idle_level = RMT_IDLE_LEVEL_LOW,
            .carrier_duty_percent = 33,
            .carrier_en = false,
            .loop_en = false,
            .idle_output_en = true,
        }
    };

    // config.clk_div = 2;

    rmt_config(&config);
    rmt_driver_install(config.channel, 0, 0);

    // Convert NS timings to ticks
    uint32_t counter_clk_hz = 0;

    rmt_get_counter_clock(config.channel, &counter_clk_hz);

    // NS to tick converter
    float ratio = (float)counter_clk_hz / 1e9;

    t0h_ticks = (uint32_t)(ratio * WS2812_T0H_NS);
    t0l_ticks = (uint32_t)(ratio * WS2812_T0L_NS);
    t1h_ticks = (uint32_t)(ratio * WS2812_T1H_NS);
    t1l_ticks = (uint32_t)(ratio * WS2812_T1L_NS);

    // Initialize automatic timing translator
    rmt_translator_init(config.channel, ws2812_rmt_adapter);

    // gpio_set_direction(NEOPIXEL_PIN, GPIO_MODE_OUTPUT);

    pixels = (uint8_t *)malloc(NEOPIXEL_SIZE * 4);

    return ESP_OK;
}

esp_err_t example_neopixel_deinitialize()
{
    // Free channel again
    rmt_driver_uninstall(config.channel);

    return ESP_OK;
}

esp_err_t example_neopixel_set_color(uint8_t red, uint8_t green, uint8_t blue, uint8_t white)
{
    for (size_t i = 0; i < NEOPIXEL_SIZE; i++)
    {
        pixels[i * 4] = red;
        pixels[i * 4 + 1] = green;
        pixels[i * 4 + 2] = blue;
        pixels[i * 4 + 3] = white;
    }

    // Write and wait to finish
    rmt_write_sample(config.channel, pixels, NEOPIXEL_SIZE * 4, true);
    rmt_wait_tx_done(config.channel, pdMS_TO_TICKS(100));

    return ESP_OK;
}

esp_err_t example_neopixel_set_pixel_color(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t)
{

    return ESP_OK;
}
