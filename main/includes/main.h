#pragma once
#include "esp_log.h"
#include "esp_system.h"

#ifdef __cplusplus
extern "C" {
#endif

static const char *APP_TAG = "8 ball";

/* Generator C calls */
// void test_gen();

esp_err_t initialiseGenerator(char *type,char *config);
// char *generateText();

/* spi file system init */
//static char *FS_BASE = "/files";
esp_err_t init_filesystem();
esp_err_t close_filesystem();

/* Screen C calls */
typedef struct {
    uint8_t clk_pin;
    uint8_t mosi_pin;
    uint8_t dc_pin;
    uint8_t cs_pin;
    uint8_t reset_pin;
    uint8_t power_pin;
    int width;
    int height;
} lcd_config;
esp_err_t init_screen(lcd_config config);

/* Accelerometer calls */
typedef struct {
    uint8_t scl_pin;
    uint8_t sda_pin;
    uint32_t clock_hz;
} mpu6050_cfg;
// uint8_t get_mpu6050_id();
esp_err_t setup_8ball_mpu6050(mpu6050_cfg config);
// double read_g_load();
esp_err_t init_generator(const char *filename);
esp_err_t load_fonts(const char* files[],size_t num_files);

void on_timer(void *args);

#ifdef __cplusplus
}
#endif