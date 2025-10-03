#include "esp_system.h"
#include "esp_spiffs.h"

extern "C" {

static char *FS_BASE = "/files";

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

}