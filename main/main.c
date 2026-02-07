#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mpu6050.h"
#include "capi.h"
#include "esp_log.h"

#define TAG "8 ball main"
#define AWAKE_SECONDS 3

mpu_config mpuConfig;
lcd_config config;
static void *screenHandle = NULL;
static void *genHandle = NULL;
static bool screenPowered = false;

void idleCallback() {
    screenPowered = !screenPowered;
    screenPower(screenHandle, screenPowered);
    if (screenPowered) {
        newText(genHandle, screenHandle);
    }
}

void shakeCallback() {
    if (!screenPowered) {
        idleCallback();
    }
    newText(genHandle, screenHandle);
}

void otherCallback(uint8_t status) {
    ESP_LOGI(TAG, "Other interrupt detected! INT_STATUS=0x%02X", status);
}

void app_main(void)
{
    #ifdef CONFIG_IDF_TARGET_ESP32C3
    readConfigFile("/files/config_c3.json", &mpuConfig, &config);
    #elif defined(CONFIG_IDF_TARGET_ESP32S3)
    readConfigFile("/files/config_s3.json", &mpuConfig, &config);
    #endif

    ESP_LOGI(TAG, "Starting GC9A01");
    screenHandle = initScreen(config);
    if (screenHandle == NULL) {
        ESP_LOGE(TAG, "Failed to initialize screen");
        return;
    }
    genHandle = initGenerator();
    if (genHandle == NULL) {
        ESP_LOGE(TAG, "Failed to initialize text generator");
        return;
    }
    newText(genHandle, screenHandle);
    for (int i=0;i<AWAKE_SECONDS;++i) {
        ESP_LOGI(TAG, "%i", AWAKE_SECONDS-i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_LOGI(TAG, "Starting MPU6050 loop");
    wake_loop(&shakeCallback, &idleCallback, &otherCallback, mpuConfig);
}



