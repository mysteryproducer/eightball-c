#include <stdio.h>
#include "mpu6050.h"
#include "esp_log.h"

void app_main(void)
{
    ESP_LOGI("MAIN", "Starting MPU6050 wake-on-motion example");
    wake_loop();
}



