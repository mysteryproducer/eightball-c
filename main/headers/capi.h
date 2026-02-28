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
    uint8_t fontCount;
    char **fontFiles;
} lcd_config;

typedef struct {
    gpio_num_t power;
    gpio_num_t interrupt;
    gpio_num_t sda;
    gpio_num_t scl;
} mpu_config;

typedef struct {
    char gen_type[12];
    char args[64];
} gen_config;

typedef enum {
    SLEEP_MODE_NONE = 0,
    SLEEP_MODE_LIGHT,
    SLEEP_MODE_DEEP
} sleep_mode_t;

void start8ball();
//MPU6050 sleep loop:
void wake_loop(void (*shakeCB)(void), void (*idleCB)(void), void (*otherCB)(uint8_t),mpu_config config);
void enable_sleep(sleep_mode_t mode);

esp_err_t readConfigFile(const char *filename,mpu_config *mpu,lcd_config *lcd,gen_config *gen);
esp_err_t init_usb_msc(void (*mountCB)(void),void (*unmountCB)(void));
esp_err_t stop_usb_msc();

#if __cplusplus
}
#endif