#include "math.h"
#include "accelerometer.h" 
#include "event_source.h"
#include "mpu6050.h"

using namespace EightBall;

Accelerometer::Accelerometer(mpu6050_cfg config,esp_err_t *result) {
    i2c_config_t i2c_cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = config.sda_pin,
        .scl_io_num = config.scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {
            .clk_speed = config.clock_hz
        }
    };
//    i2c_cfg.master
    esp_err_t setup_result=i2c_param_config(I2C_CHANNEL,&i2c_cfg);
    if (setup_result==ESP_OK) {
        setup_result=i2c_driver_install(I2C_CHANNEL,I2C_MODE_MASTER,0,0,0);
    }

    if (setup_result==ESP_OK) {
        this->setup_8ball_mpu6050();
        uint8_t address;
        esp_err_t result = mpu6050_get_deviceid(this->accelerometer, &address);
        ESP_LOGI(TAG, "Connected to MPU6050: %i", address);
    }
    *result=setup_result;
}

esp_err_t Accelerometer::setup_8ball_mpu6050() {
    this->accelerometer=mpu6050_create(I2C_CHANNEL,MPU6050_I2C_ADDRESS);
    esp_err_t result=mpu6050_config(this->accelerometer,ACCE_FS_2G,GYRO_FS_250DPS);
    if (result==ESP_OK) {
        result=this->wakeUp();
    }
    return result;
}

// uint8_t Accelerometer::getDeviceID() {
//     uint8_t address;
//     esp_err_t result = mpu6050_get_deviceid(this->accelerometer, &address);
//     return (result==ESP_OK)?address:0;
// }

esp_err_t Accelerometer::wakeUp() {
    return mpu6050_wake_up(this->accelerometer);
}

esp_err_t Accelerometer::sleep() {
    return mpu6050_sleep(this->accelerometer);
}

double Accelerometer::getGLoad() {
    mpu6050_acce_value_t accel;
    if (mpu6050_get_acce(accelerometer,&accel)==ESP_OK) {
        double squares = accel.acce_x*accel.acce_x+accel.acce_y*accel.acce_y+accel.acce_z*accel.acce_z;
        double g_load=sqrt(squares);
        return g_load;
    }
    return NAN;
}