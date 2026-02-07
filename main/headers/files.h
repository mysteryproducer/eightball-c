#pragma once
#include "capi.h"
#include "esp_err.h"
#include "esp_spiffs.h"

//const char *FS_BASE = "/files";
#define FS_BASE "/files"

esp_err_t init_filesystem();

esp_err_t close_filesystem();

esp_err_t readConfigFile(mpu_config *mpu,lcd_config *lcd);