/*
primary source:
https://esp32tutorials.com/esp32-spiffs-esp-idf/
*/
#include "capi.h"
#include "files.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_vfs_fat.h"
#include "json_parser.h"
#include <string>
#include <fstream>

#define TAG "file utils" 
static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

esp_err_t init_filesystem() {
    esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
    };

    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(FS_BASE, "storage", &mount_config, &s_wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "FAT mounted at %s", FS_BASE);
    return err;
}

esp_err_t close_filesystem() {
//    return esp_vfs_spiffs_unregister(NULL);
    return esp_vfs_fat_spiflash_unmount_rw_wl(FS_BASE, s_wl_handle);
}

std::string readFileToString(const std::string& path) {
    std::ifstream file(path);
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

esp_err_t readConfigFile(const char *filename,mpu_config *mpu,lcd_config *lcd) {
    ESP_LOGD(TAG,"Reading %s",filename);
    if (init_filesystem() != ESP_OK) {
        ESP_LOGE(TAG,"File system setup fail!");
        return ESP_FAIL;
    }

    std::string data = readFileToString(filename);
    jparse_ctx_t context;
    json_parse_start(&context,data.c_str(),data.length());

    int temp=0;
    json_obj_get_int(&context,"mpu_power",&temp);
    mpu->power = (gpio_num_t)temp;
    json_obj_get_int(&context,"interrupt",&temp);
    mpu->interrupt = (gpio_num_t)temp;
    json_obj_get_int(&context,"sda",&temp);
    mpu->sda = (gpio_num_t)temp;
    json_obj_get_int(&context,"scl",&temp);
    mpu->scl = (gpio_num_t)temp;

    json_obj_get_int(&context,"clk",&temp);
    lcd->clk_pin = (gpio_num_t)temp;
    json_obj_get_int(&context,"mosi",&temp);
    lcd->mosi_pin = (gpio_num_t)temp;
    json_obj_get_int(&context,"dc",&temp);
    lcd->dc_pin = (gpio_num_t)temp;
    json_obj_get_int(&context,"cs",&temp);
    lcd->cs_pin = (gpio_num_t)temp;
    json_obj_get_int(&context,"reset",&temp);
    lcd->reset_pin = (gpio_num_t)temp;
    json_obj_get_int(&context,"screen_power",&temp);
    lcd->power_pin = (gpio_num_t)temp;
    json_obj_get_int(&context,"width",&temp);
    lcd->width = (uint16_t)temp;
    json_obj_get_int(&context,"height",&temp);
    lcd->height = (uint16_t)temp;
    json_obj_get_int(&context,"spifreq_hz",&temp);
    lcd->freq_hz = (uint32_t)temp;

    json_parse_end(&context);

    close_filesystem();
    return ESP_OK;
}