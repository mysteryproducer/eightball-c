#pragma once

#include "esp_lcd_gc9a01.h"
#include "driver/gpio.h"
#include <stdint.h>

#if __cplusplus
extern "C" {
#endif

/* C calls */
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

typedef struct {
    gpio_num_t power;
    gpio_num_t interrupt;
    gpio_num_t sda;
    gpio_num_t scl;
} mpu_config;

//GC9A01 init/paint functions.
void * initScreen(lcd_config config);
void * initGenerator();
void newText(void *genHandle, void *screenHandle);
void screenPower(void *screenHandle, bool powerOn);
//MPU6050 sleep loop:
void wake_loop(void (*shakeCB)(void), void (*idleCB)(void), void (*otherCB)(uint8_t),mpu_config config);

#if __cplusplus
}
#endif