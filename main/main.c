#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mpu6050.h"
#include "capi.h"
#include "esp_log.h"

#define TAG "8 ball main"
#define AWAKE_SECONDS 3

#ifdef CONFIG_IDF_TARGET_ESP32C3
mpu_config mpuConfig = {
    .power = GPIO_NUM_5,
    .interrupt = GPIO_NUM_21,
    .scl = GPIO_NUM_7,
    .sda = GPIO_NUM_8
};
lcd_config config = {
    .clk_pin = GPIO_NUM_0,
    .mosi_pin = GPIO_NUM_1,
    .cs_pin = GPIO_NUM_3,
    .dc_pin = GPIO_NUM_2,
    
    .reset_pin = GPIO_NUM_4,
    .power_pin = GPIO_NUM_10,  
    .width = 240,
    .height = 240,
    .freq_hz = 40 * 1000 * 1000
};
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
mpu_config mpuConfig = {
    .power = GPIO_NUM_NC,
    .interrupt = GPIO_NUM_8,
    .scl = GPIO_NUM_13,
    .sda = GPIO_NUM_12
};
lcd_config config = {
    .clk_pin = GPIO_NUM_1,
    .mosi_pin = GPIO_NUM_2,
    .cs_pin = GPIO_NUM_4,
    .dc_pin = GPIO_NUM_3,
    
    .reset_pin = GPIO_NUM_5,
    .power_pin = GPIO_NUM_6,
    .width = 240,
    .height = 240,
    .freq_hz = 40 * 1000 * 1000
};
#endif
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



