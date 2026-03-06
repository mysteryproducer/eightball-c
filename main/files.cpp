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
        .max_files = 5
    };

    //esp_err_t err = esp_vfs_fat_spiflash_mount_ro(FS_BASE, "storage", &mount_config);//, &s_wl_handle);
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

esp_err_t readDetectionConfig(jparse_ctx_t *context, motion_detection_config *config) {
    json_obj_get_int(context,"threshold",(int*)&config->threshold);
    json_obj_get_int(context,"duration",(int*)&config->duration);
    return ESP_OK;
}

esp_err_t readMotionConfig(jparse_ctx_t *context, mpu_config *mpu) {
    ESP_LOGD(TAG,"Reading motion config ");

    json_obj_get_int(context,"mpu_power",(int*)&mpu->power);
    json_obj_get_int(context,"interrupt",(int*)&mpu->interrupt);
    json_obj_get_int(context,"sda",(int*)&mpu->sda);
    json_obj_get_int(context,"scl",(int*)&mpu->scl);

    if (json_obj_get_object(context,"motion") == OS_SUCCESS) {
        ESP_LOGD(TAG,"Reading motion detection config");
        readDetectionConfig(context, &mpu->motion);
        json_obj_leave_object(context);
    } else {
        ESP_LOGW(TAG,"No motion detection config found, using defaults");
        mpu->motion.threshold = 6;
        mpu->motion.duration = 3;
    }
    if (json_obj_get_object(context,"zero_motion") == OS_SUCCESS) {
        ESP_LOGD(TAG,"Reading zero-motion detection config");
        readDetectionConfig(context, &mpu->zero_motion);
        json_obj_leave_object(context);
    } else {
        ESP_LOGW(TAG,"No zero-motion detection config found, using defaults");
        mpu->zero_motion.threshold = 4;
        mpu->zero_motion.duration = 5;
    }
    return ESP_OK;
}

esp_err_t readScreenConfig(jparse_ctx_t *context, lcd_config *lcd) {
    json_obj_get_int(context,"clk",(int*)&lcd->clk_pin);
    json_obj_get_int(context,"mosi",(int*)&lcd->mosi_pin);
    json_obj_get_int(context,"dc",(int*)&lcd->dc_pin);
    json_obj_get_int(context,"cs",(int*)&lcd->cs_pin);
    json_obj_get_int(context,"reset",(int*)&lcd->reset_pin);
    json_obj_get_int(context,"screen_power",(int*)&lcd->power_pin);
    json_obj_get_int(context,"width",(int*)&lcd->width);
    json_obj_get_int(context,"height",(int*)&lcd->height);
    json_obj_get_int(context,"spifreq_hz",(int*)&lcd->freq_hz);
    json_obj_get_array(context,"fonts",(int*)&lcd->fontCount);
    lcd->fontFiles = new char*[lcd->fontCount];
    for (int i=0;i<lcd->fontCount;++i) {
        char fontPath[64];
        json_arr_get_string(context,i,fontPath,64);
        lcd->fontFiles[i] = new char[strlen(fontPath)+1];
        strncpy(lcd->fontFiles[i], fontPath, strlen(fontPath)+1);
    }
    json_obj_leave_array(context);
    return ESP_OK;
}

esp_err_t readGeneratorConfig(jparse_ctx_t *context, gen_config *gen) {
    json_obj_get_string(context,"type",gen->gen_type,12);
    json_obj_get_array(context,"args",(int*)&gen->arg_count);
    gen->args = new char*[gen->arg_count];
    for (int i=0;i<gen->arg_count;++i) {
        char arg_value[64];
        json_arr_get_string(context,i,arg_value,64);
        gen->args[i] = new char[strlen(arg_value)+1];
        strncpy(gen->args[i], arg_value, strlen(arg_value)+1);
    }
    json_obj_leave_array(context);
    return ESP_OK;
}

esp_err_t readConfigFile(const char *filename,mpu_config *mpu,lcd_config *lcd,gen_config *gen) {
    ESP_LOGD(TAG,"Reading %s",filename);
    if (init_filesystem() != ESP_OK) {
        ESP_LOGE(TAG,"File system setup fail!");
        return ESP_FAIL;
    }

    std::string data = readFileToString(filename);
    jparse_ctx_t context;
    if (json_parse_start(&context,data.c_str(),data.length()) != OS_SUCCESS) {
        ESP_LOGE(TAG,"Failed to parse config file");
        close_filesystem();
        return ESP_FAIL;
    }

    if (json_obj_get_object(&context,"accelerometer") == OS_SUCCESS) {
        readMotionConfig(&context, mpu);
        json_obj_leave_object(&context);
    }
    if (json_obj_get_object(&context,"screen") == OS_SUCCESS) {
        readScreenConfig(&context, lcd);
        json_obj_leave_object(&context);
    }
    if (json_obj_get_object(&context,"generator") == OS_SUCCESS) {
        readGeneratorConfig(&context, gen);
        json_obj_leave_object(&context);
    }

    json_parse_end(&context);

    close_filesystem();
    return ESP_OK;
}