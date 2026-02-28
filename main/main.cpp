#include <exception>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mpu6050.h"
#include "tft.h"
#include "gen.h"
#include "capi.h"
#include "files.h"
#include "esp_log.h"

#define TAG "8 ball main"
#define AWAKE_SECONDS 2
using namespace EightBall;

mpu_config mpuConfig;
lcd_config config;
gen_config genConfig;
static EightBallScreen *screenHandle = NULL;
static TextGenerator *genHandle = NULL;
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

static bool msc_mounted = false;
void usb_mounted() {
    ESP_LOGI(TAG, "USB mounted");
    msc_mounted=true;
    enable_sleep(SLEEP_MODE_NONE);
    showMessage(screenHandle, "USB Storage mode");
}
void usb_unmounted() {
    ESP_LOGI(TAG, "USB unmounted");
    msc_mounted=false;
    enable_sleep(SLEEP_MODE_LIGHT);
}

void start8ball(void) {
    try {
        readConfigFile("/files/config.json", &mpuConfig, &config, &genConfig);
        ESP_LOGI(TAG, "Starting GC9A01");
        screenHandle = initScreen(config);
        if (screenHandle == NULL) {
            ESP_LOGE(TAG, "Failed to initialize screen");
            return;
        }
        genHandle = (TextGenerator *)initGenerator(genConfig);
        if (genHandle == NULL) {
            ESP_LOGE(TAG, "Failed to initialize text generator");
            return;
        }
        newText(genHandle, screenHandle);
    } catch (const std::exception &e) {
        ESP_LOGE(TAG, "Exception during initialization: %s", e.what());
    }
    ESP_LOGI(TAG, "Starting USB MSC; eject the device to enable motion detection");
    init_usb_msc(&usb_mounted, &usb_unmounted);
    for (int i=0;msc_mounted || (i<AWAKE_SECONDS);++i) {
        ESP_LOGI(TAG, "%i", AWAKE_SECONDS-i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    stop_usb_msc();
    // ESP_LOGI(TAG, "Starting USB MSC; eject the device to enable motion detection");
    // init_usb_msc(&usb_mounted, &usb_unmounted);
    ESP_LOGI(TAG, "Starting MPU6050 loop");
    wake_loop(&shakeCallback, &idleCallback, &otherCallback, mpuConfig);
}