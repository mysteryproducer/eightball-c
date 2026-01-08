#pragma once
#if __cplusplus
extern "C" {
#endif
// Custom GC9A01 init sequence
#include "esp_lcd_gc9a01.h"
#include "driver/gpio.h"

/* Screen C calls */
typedef struct {
    gpio_num_t clk_pin;
    gpio_num_t mosi_pin;
    gpio_num_t dc_pin;
    gpio_num_t cs_pin;
    gpio_num_t reset_pin;
    gpio_num_t power_pin;
    uint16_t width;
    uint16_t height;
    uint32_t freq_hz;
} lcd_config;

void * initScreen(lcd_config config);
void * initGenerator();
void newText(void *genHandle, void *screenHandle);
void screenPower(void *screenHandle, bool powerOn);

#if __cplusplus
}
#endif