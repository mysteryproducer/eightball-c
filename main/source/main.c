/* esp_event (event loop library) basic example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "math.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "driver/i2c.h"
#include "tft.h"
#include "main.h"
#include "gen.h"

#include "mpu6050.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "event_source.h"
// #include <projdefs.h>
#include "pins.h"


/* Event source periodic timer related definitions */
ESP_EVENT_DEFINE_BASE(TIMER_EVENTS);
/* Event source task related definitions */
//ESP_EVENT_DEFINE_BASE(TASK_EVENTS);
esp_timer_handle_t TIMER;
mpu6050_handle_t accelerometer;

esp_err_t setup_8ball_i2c() {
    i2c_config_t i2c_cfg = {
        .mode=I2C_MODE_MASTER,
        .sda_io_num=PIN_I2C_SDA,
        .scl_io_num=PIN_I2C_SCL,
        .master.clk_speed=I2C_CLOCK,
        .sda_pullup_en=GPIO_PULLUP_ENABLE,
        .scl_pullup_en=GPIO_PULLUP_ENABLE
    };
    esp_err_t setup_result=i2c_param_config(I2C_NUM_0,&i2c_cfg);
    if (setup_result==ESP_OK) {
        setup_result=i2c_driver_install(I2C_NUM_0,I2C_MODE_MASTER,0,0,0);
    }
    return setup_result;
}

esp_err_t setup_8ball_mpu6050() {
    accelerometer=mpu6050_create(I2C_NUM_0,MPU6050_I2C_ADDRESS);
    esp_err_t result=mpu6050_config(accelerometer,ACCE_FS_2G,GYRO_FS_250DPS);
    if (result==ESP_OK) {
        result=mpu6050_wake_up(accelerometer);
    }
    return result;
}

// to the default event loop.
static void t_cb(void* arg)
{
    mpu6050_acce_value_t accel;
    if (mpu6050_get_acce(accelerometer,&accel)==ESP_OK) {
        float g_load=sqrt(accel.acce_x*accel.acce_x+accel.acce_y*accel.acce_y+accel.acce_z*accel.acce_z);
        if (g_load>1.3) {
            ESP_LOGI("timer", "%f %f %f", accel.acce_x, accel.acce_y, accel.acce_z);
        }
    }
    draw_8ball_blank_blue();
}

/* Example main */
void app_main(void)
{
//    esp_timer_handle_t TIMER;
//    esp_timer_create
    //set the power switch pin high
    setup_8ball_power_pin();
    set_8ball_tft_power(1);

    if (setup_8ball_i2c()!=ESP_OK) {
        ESP_LOGW(TAG, "Failed to initialise I2C interface");
    }
    if (setup_8ball_mpu6050()!=ESP_OK) {
        ESP_LOGW(TAG, "Failed to initialise MPU6050");
    } else {
        uint8_t device_id;
        mpu6050_get_deviceid(accelerometer,&device_id);
        ESP_LOGI(TAG, "Connected to MPU6050: %i", device_id);
    }

    if (setup_8ball_tft()!=ESP_OK) {
        ESP_LOGW(TAG, "Failed to initialise TFT");
    }
    draw_8ball_blank_blue();

    esp_timer_create_args_t timer_args = {
        .callback = &t_cb,
    };
//    gpio_dump_io_configuration(stdout,0x0FFFFF);

    esp_timer_create(&timer_args, &TIMER); 
    esp_timer_start_periodic(TIMER, TIMER_PERIOD);
    esp_event_loop_create_default();
}

/* Handler which executes when the timer stopped event gets executed by the loop. Since the timer has been
// stopped, it is safe to delete it.
static void timer_stopped_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    ESP_LOGI("timer", "%s: timer_stopped_handler", base);
    // Delete the timer
    esp_timer_delete(TIMER);
    ESP_LOGI("timer", "%s: deleted timer event source", base);
}
*/