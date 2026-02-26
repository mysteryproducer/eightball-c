#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mpu6050.h"
#include "capi.h"
#include "esp_log.h"

#define TAG "8 ball main"
#define AWAKE_SECONDS 5

mpu_config mpuConfig;
lcd_config config;
gen_config genConfig;
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

void usb_mounted() {
    ESP_LOGI(TAG, "USB mounted");
    enable_sleep(SLEEP_MODE_NONE);
}
void usb_unmounted() {
    ESP_LOGI(TAG, "USB unmounted");
    enable_sleep(SLEEP_MODE_LIGHT);
}

void app_main(void)
{
    readConfigFile("/files/config.json", &mpuConfig, &config, &genConfig);
    ESP_LOGI(TAG, "Starting USB MSC; eject the device to enable motion detection");
    init_usb_msc(&usb_mounted, &usb_unmounted);

    ESP_LOGI(TAG, "Starting GC9A01");
    screenHandle = initScreen(config);
    if (screenHandle == NULL) {
        ESP_LOGE(TAG, "Failed to initialize screen");
        return;
    }
    genHandle = initGenerator(genConfig);
    if (genHandle == NULL) {
        ESP_LOGE(TAG, "Failed to initialize text generator");
        return;
    }
    newText(genHandle, screenHandle);
    for (int i=0;i<AWAKE_SECONDS;++i) {
        ESP_LOGI(TAG, "%i", AWAKE_SECONDS-i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    // ESP_LOGI(TAG, "Starting USB MSC; eject the device to enable motion detection");
    // init_usb_msc(&usb_mounted, &usb_unmounted);
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Starting MPU6050 loop");
    wake_loop(&shakeCallback, &idleCallback, &otherCallback, mpuConfig);
}



