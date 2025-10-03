#pragma once
#include "main.h"
#include "driver/i2c_master.h"
#include "mpu6050.h"

namespace EightBall {
#define I2C_CHANNEL I2C_NUM_0

class Accelerometer {
    public:
        Accelerometer(mpu6050_cfg config,esp_err_t *result);
        uint8_t getDeviceID();
        double getGLoad();
        esp_err_t wakeUp();
        esp_err_t sleep();
    private:
//        i2c_master_bus_handle_t i2c_handle;
        mpu6050_handle_t accelerometer;
        uint8_t device_id;

        esp_err_t setup_8ball_mpu6050();
};

}