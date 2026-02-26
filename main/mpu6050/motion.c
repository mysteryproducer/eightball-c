/* key sources:
https://www.eluke.nl/2016/08/11/how-to-enable-motion-detection-interrupt-on-mpu6050/
https://www.i2cdevlib.com/devices/mpu6050#registers

https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/sleep_modes.html
*/

#include "capi.h"

#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_sleep.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_sleep.h"

#define I2C_MASTER_NUM         I2C_NUM_0
#define I2C_MASTER_FREQ_HZ     400000
#define I2C_MASTER_TIMEOUT_MS  1000
#define MPU6050_ADDR           0x68

#define MPU6050_REG_PWR_MGMT_1         0x6B
#define MPU6050_REG_PWR_MGMT_2         0x6C
#define MPU6050_REG_INT_ENABLE         0x38
#define MPU6050_REG_INT_STATUS         0x3A
#define MPU6050_REG_INT_PIN_CFG        0x37
#define MPU6050_REG_MOT_THR            0x1F
#define MPU6050_REG_MOT_DUR            0x20
#define MPU6050_REG_ZRMOT_THR          0x21
#define MPU6050_REG_ZRMOT_DUR          0x22
#define MPU6050_REG_MOTION_DETECT_CTRL    0x69
#define MPU6050_REG_ACCEL_CONFIG       0x1C
#define MPU6050_REG_SIGNAL_PATH_RESET  0x68

#define MPU6050_MOTION_INT_BIT         0x40
#define MPU6050_ZERMOT_INT_BIT         0x20

#define TAG                            "MPU6050"
static i2c_master_bus_handle_t i2c_handle;
static i2c_master_dev_handle_t mpu6050_handle;
static sleep_mode_t sleepMode = SLEEP_MODE_LIGHT;

static void i2c_write_byte(uint8_t reg, uint8_t data) {
    uint8_t buf[2] = {reg, data};
    ESP_ERROR_CHECK(i2c_master_transmit(mpu6050_handle, buf, 2, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS)));
}

static void i2c_read_byte(uint8_t reg, uint8_t *data) {
    i2c_master_transmit_receive(mpu6050_handle, &reg, 1, data, 1, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
}

static void i2c_master_init(mpu_config config) {
    i2c_master_bus_config_t conf = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .sda_io_num = config.sda,
        .scl_io_num = config.scl,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
        .flags.allow_pd = false
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&conf, &i2c_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MPU6050_ADDR,
        .scl_speed_hz = 400000
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_handle, &dev_cfg, &mpu6050_handle));
}

static void mpu6050_enter_wake_on_motion(void) {
    ESP_LOGI(TAG, "Configuring MPU6050 for low-power wake-on-motion");

    // Wake up device
    i2c_write_byte(MPU6050_REG_PWR_MGMT_1, 0x01);
    vTaskDelay(pdMS_TO_TICKS(10));


    // Reset signal paths
    i2c_write_byte(MPU6050_REG_SIGNAL_PATH_RESET, 0x07);
    vTaskDelay(pdMS_TO_TICKS(50));
    // Interrupt pin configuration: latch until cleared, active high
    i2c_write_byte(MPU6050_REG_INT_PIN_CFG, 0x20);

    // Set accelerometer range ±2g
    i2c_write_byte(MPU6050_REG_ACCEL_CONFIG, 0x00);
    // Motion threshold: lower = more sensitive
    i2c_write_byte(MPU6050_REG_MOT_THR, 6);
    i2c_write_byte(MPU6050_REG_MOT_DUR, 3); // 3*1ms=3ms
    // Motion threshold: lower = more sensitive
    i2c_write_byte(MPU6050_REG_ZRMOT_THR, 4);
    // i2c_write_byte(MPU6050_REG_ZRMOT_DUR, 0x78); // 0x78*64ms=5s
    // duration is in greater increments during cycle mode.
    i2c_write_byte(MPU6050_REG_ZRMOT_DUR, 0x05);

    // Configure motion detection control:
    // Enable accelerometer hardware for motion detection
    i2c_write_byte(MPU6050_REG_MOTION_DETECT_CTRL, 0x15); // 0x15 = latch, compare filtered data

    // Enable motion interrupt
    i2c_write_byte(MPU6050_REG_INT_ENABLE, MPU6050_MOTION_INT_BIT | MPU6050_ZERMOT_INT_BIT);

    // Enable low-power accelerometer mode (cycle mode)
    // Sleep bit=0, Cycle=1, use internal 1.25Hz sampling0
    // bits 6-7: 0=1.25Hz, 1=2.5Hz, 2=5Hz, 3=10Hz. Go with 5Hz
    i2c_write_byte(MPU6050_REG_PWR_MGMT_2, 0x87); // disable gyro axes, keep accel only
    // bit5=1 → cycle mode, bit 3=1 → temperature sensor disable
    i2c_write_byte(MPU6050_REG_PWR_MGMT_1, 0x28);

    ESP_LOGI(TAG, "MPU6050 in low-power motion detection mode");
}

static void clear_motion_interrupt(void) {
    uint8_t tmp;
    i2c_read_byte(MPU6050_REG_INT_STATUS, &tmp);
}

void powerMPU6050(gpio_num_t powerPin) {
    if (powerPin != GPIO_NUM_NC) {
        gpio_reset_pin(powerPin);
        gpio_set_direction(powerPin, GPIO_MODE_OUTPUT);
        gpio_set_level(powerPin, 1);
        gpio_hold_en(powerPin);
    }
}

void enable_sleep(sleep_mode_t mode) {
    sleepMode = mode;
}

void wake_loop(void (*shakeCB)(void), void (*idleCB)(void), void (*otherCB)(uint8_t),mpu_config config) {
    powerMPU6050(config.power);
    vTaskDelay(pdMS_TO_TICKS(100)); // wait for power to stabilize
    i2c_master_init(config);

    gpio_reset_pin(config.interrupt);
    gpio_set_direction(config.interrupt, GPIO_MODE_INPUT);
    gpio_pullup_en(config.interrupt);
    gpio_pulldown_dis(config.interrupt);

    mpu6050_enter_wake_on_motion();
    bool deepSleep=false;

    while (true) {
        clear_motion_interrupt();
        if (sleepMode == SLEEP_MODE_NONE) {
            vTaskDelay(pdMS_TO_TICKS(100)); // Poll every 0.1s
            continue;
        }
        if (!deepSleep || sleepMode == SLEEP_MODE_LIGHT) {
            ESP_LOGD(TAG,"Light sleep");
            gpio_wakeup_enable(config.interrupt, GPIO_INTR_HIGH_LEVEL);
            esp_sleep_enable_gpio_wakeup();
            esp_light_sleep_start();
        } else {
            ESP_LOGD(TAG,"Deep sleep");
            //TODO: make sure we can wake from deep sleep with the MPU6050 interrupt. 
            //This may require additional hardware (e.g. a transistor to pull the interrupt pin low when motion is detected, 
            //since the ESP32 can't wake from deep sleep on a pin that goes high). 
            //        <- this from AI suggestion, not tested yet.
//            esp_err_t wakeConfigured = esp_deep_sleep_enable_gpio_wakeup(1 << config.interrupt, GPIO_INTR_HIGH_LEVEL); // Wake on HIGH
            // if (wakeConfigured == ESP_OK) {
            //     esp_deep_sleep_start();
            //     //reinit screen on wake?
            // } else {
            //     deepSleep=false;
            // }
        }

        uint8_t status;
        i2c_read_byte(MPU6050_REG_INT_STATUS, &status); 
        if (status & MPU6050_MOTION_INT_BIT) {
            ESP_LOGD(TAG, "Motion interrupt");
            shakeCB();
        } else if (status & MPU6050_ZERMOT_INT_BIT) {
            ESP_LOGD(TAG, "Zero-motion interrupt");
            idleCB();
            deepSleep = !deepSleep;
        } else if (status == 0) {
            continue; // No interrupt, likely a spurious wakeup or in USB mode. Ignore.
        } else {
            ESP_LOGI(TAG, "Woke for unknown reason, INT_STATUS=0x%02X", status);
            otherCB(status);
        }
    }
}
