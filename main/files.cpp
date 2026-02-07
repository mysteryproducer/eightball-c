/*
primary source:
https://esp32tutorials.com/esp32-spiffs-esp-idf/
*/
#include "files.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_spiffs.h"
#include "json_parser.h"
#include <string>
#include <fstream>

#define TAG "file utils" 

esp_err_t init_filesystem() {
    esp_vfs_spiffs_conf_t config = {
        .base_path = FS_BASE,
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true,
    };
    return esp_vfs_spiffs_register(&config);
}

esp_err_t close_filesystem() {
    return esp_vfs_spiffs_unregister(NULL);
}

std::string readFileToString(const std::string& path) {
    std::ifstream file(path);
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

esp_err_t readConfigFile(mpu_config *mpu,lcd_config *lcd) {
    ESP_LOGI(TAG,"Reading config.json");
    if (init_filesystem() != ESP_OK) {
        ESP_LOGE(TAG,"File system setup fail!");
        return ESP_FAIL;
    }

    std::string data = readFileToString("config.json");
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
/*    mpu->power = data["mpu_power"].get<gpio_num_t>();
    mpu->interrupt = data["interrupt"].get<gpio_num_t>();
    mpu->sda = data["sda"].get<uint8_t>();
    mpu->scl = data["scl"].get<uint8_t>();

    lcd->clk_pin = data["clk"].get<uint8_t>();
    lcd->mosi_pin = data["mosi"].get<uint8_t>();
    lcd->dc_pin = data["dc"].get<uint8_t>();
    lcd->cs_pin = data["cs"].get<uint8_t>();
    lcd->reset_pin = data["reset"].get<uint8_t>();
    lcd->power_pin = data["screen_power"].get<uint8_t>();
    lcd->width = data["width"].get<uint16_t>();
    lcd->height = data["height "].get<uint16_t>();
    lcd->freq_hz = data["spifreq_hz"].get<uint32_t>();*/
    return ESP_OK;
}