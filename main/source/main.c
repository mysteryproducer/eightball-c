/* esp_event (event loop library) basic example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "main.h"
#include "esp_log.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "event_source.h"
#include "pins.h"


/* Event source periodic timer related definitions */
ESP_EVENT_DEFINE_BASE(TIMER_EVENTS);
/* Event source task related definitions */
//ESP_EVENT_DEFINE_BASE(TASK_EVENTS);
esp_timer_handle_t TIMER;

// to the default event loop.
// static void t_cb(void* arg)
// {
//     double g_load=read_g_load();
//     if (g_load>1.3) {
//         ESP_LOGI("timer", "%f", g_load);
//     }
//     draw_blank_blue();
// }

/* Example main */
void app_main(void)
{
//    esp_timer_handle_t TIMER;
//    esp_timer_create
    //set the power switch pin high
    //setup_8ball_power_pin();
    lcd_config screenCfg = {
        .clk_pin = PIN_SPI_CLK,
        .mosi_pin = PIN_SPI_MOSI,
        .cs_pin = PIN_SPI_CS,
        .dc_pin = PIN_SPI_DC,
        .reset_pin = PIN_SPI_RESET,
        .power_pin = PIN_TFT_POWER,
        .width = 240,
        .height = 240
    };
    if (init_screen(screenCfg)!=ESP_OK) {
        ESP_LOGW(TAG, "Failed to initialise screen");
    }
//    set_screen_power(true);
    mpu6050_cfg accel_cfg = {
        .scl_pin = PIN_I2C_SCL,
        .sda_pin = PIN_I2C_SDA,
        .clock_hz = I2C_CLOCK
    };
    if (setup_8ball_mpu6050(accel_cfg)!=ESP_OK) {
        ESP_LOGW(TAG, "Failed to initialise MPU6050");
    } else {
//        uint8_t device_id = get_mpu6050_id();
//        ESP_LOGI(TAG, "Connected to MPU6050: %i", device_id);
    }
    init_generator("/files/dsm5.txt");
    const char *files[3];
    files[0]="/files/monaco14.bin";
    files[1]="/files/monaco20.bin";
    files[2]="/files/monaco28.bin";
    load_fonts(files,3);

//    draw_blank_blue();

    esp_timer_create_args_t timer_args = {
        .callback = &on_timer,
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