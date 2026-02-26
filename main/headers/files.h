#pragma once
#include "capi.h"
#include "esp_err.h"

#define FS_BASE "/files"

esp_err_t init_filesystem();

esp_err_t close_filesystem();