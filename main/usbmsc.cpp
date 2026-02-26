#include <errno.h>
#include <dirent.h>
#include "capi.h"
#include "files.h"
#include "esp_log.h"
#include "esp_system.h"
#include "tusb.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "tinyusb_msc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "USB MSC"

//wear leveling handle for USB MSC storage
static wl_handle_t wl_handle = WL_INVALID_HANDLE;

static void (*mount_callback)(void) = NULL;
static void (*unmount_callback)(void) = NULL;

static tinyusb_msc_storage_handle_t msc_handle = NULL;
/* TinyUSB declarations */
#define EPNUM_MSC       1
#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)
enum {
    ITF_NUM_MSC = 0,
    ITF_NUM_TOTAL
};

enum {
    EDPT_CTRL_OUT = 0x00,
    EDPT_CTRL_IN  = 0x80,

    EDPT_MSC_OUT  = 0x01,
    EDPT_MSC_IN   = 0x81,
};

static tusb_desc_device_t descriptor_config = {
    .bLength = sizeof(descriptor_config),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0x303A, // This is Espressif VID. This needs to be changed according to Users / Customers
    .idProduct = 0x4002,
    .bcdDevice = 0x100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01
};

static uint8_t const msc_fs_configuration_desc[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Interface number, string index, EP Out & EP In address, EP size
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 0, EDPT_MSC_OUT, EDPT_MSC_IN, 64),
};
static char const *string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 },  // 0: is supported language is English (0x0409)
    "TinyUSB",                      // 1: Manufacturer
    "Magic 8 Ball",                 // 2: Product
    "888888",                       // 3: Serials
    "Magic 8 Ball Mass Storage",    // 4. MSC
};
/*
void __attribute__((weak)) tud_mount_cb(void) { ESP_LOGI(TAG, "TinyUSB: mounted"); }
void __attribute__((weak)) tud_umount_cb(void) { ESP_LOGI(TAG, "TinyUSB: unmounted"); }
void __attribute__((weak)) tud_suspend_cb(bool remote_wakeup_en) { ESP_LOGI(TAG, "TinyUSB: suspended (remote_wakeup=%d)", remote_wakeup_en); }
void __attribute__((weak)) tud_resume_cb(void) { ESP_LOGI(TAG, "TinyUSB: resumed"); }
*/

static esp_err_t storage_init_spiflash(wl_handle_t *wl_handle)
{
    ESP_LOGI(TAG, "Initializing wear levelling");

    const esp_partition_t *data_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, NULL);
    if (data_partition == NULL) {
        ESP_LOGE(TAG, "Failed to find FATFS partition. Check the partition table.");
        return ESP_ERR_NOT_FOUND;
    }

    return wl_mount(data_partition, wl_handle);
}

/**
 * @brief Storage mount changed callback
 *
 * @param handle Storage handle
 * @param event Event information
 * @param arg User argument, provided during callback registration
 */
static void storage_mount_changed_cb(tinyusb_msc_storage_handle_t handle, tinyusb_msc_event_t *event, void *arg)
{
    if (event->id == TINYUSB_MSC_EVENT_MOUNT_COMPLETE) {
        ESP_LOGI(TAG, "Storage mount changed: %s", (event->mount_point == TINYUSB_MSC_STORAGE_MOUNT_APP) ? "Mounted to APP" : "Mounted to USB");
        if event->is_mounted {
            mount_callback();
        } else {
            unmount_callback();
        }
    }
}

// Set mount point to the application and list files in BASE_PATH by filesystem API
static void _mount(void)
{
    ESP_LOGI(TAG, "Mount storage...");
    ESP_ERROR_CHECK(tinyusb_msc_set_storage_mount_point(msc_handle, TINYUSB_MSC_STORAGE_MOUNT_APP));

    // List all the files in this directory
    ESP_LOGI(TAG, "\nls command output:");
    struct dirent *d;
    DIR *dh = opendir(FS_BASE);
    if (!dh) {
        if (errno == ENOENT) {
            // If the directory is not found
            ESP_LOGE(TAG, "Directory doesn't exist %s", FS_BASE);
        } else {
            // If the directory is not readable then throw error and exit
            ESP_LOGE(TAG, "Unable to read directory %s", FS_BASE);
        }
        return;
    }
    // While the next entry is not readable we will print directory files
    while ((d = readdir(dh)) != NULL) {
        printf("%s\n", d->d_name);
    }
    return;
}

esp_err_t init_usb_msc(void (*mountCB)(void),void (*unmountCB)(void)) {
    mount_callback = mountCB;
    unmount_callback = unmountCB;
    ESP_ERROR_CHECK(storage_init_spiflash(&wl_handle));
    // Configure the callback for mount changed events
    tinyusb_msc_storage_config_t storage_cfg = {
        .fat_fs = {
            .base_path = NULL,
            .config = {
                .format_if_mount_failed = false,
                .max_files = 5
            },
        },
        .mount_point = TINYUSB_MSC_STORAGE_MOUNT_USB,       // Initial mount point to USB
    };
    // Set the storage medium to the wear leveling handle
    storage_cfg.medium.wl_handle = wl_handle;

//    tinyusb_msc_storage_init_spiflash(&storage_cfg, &msc_handle);
    esp_err_t err = tinyusb_msc_new_storage_spiflash(&storage_cfg, &msc_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize USB MSC storage: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "MSC storage created (handle=%p)", msc_handle);
    ESP_ERROR_CHECK(tinyusb_msc_set_storage_callback(storage_mount_changed_cb, NULL));
    // Re-mount to the APP
    //_mount();

    tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();

    tusb_cfg.descriptor.device = &descriptor_config;
    tusb_cfg.descriptor.full_speed_config = msc_fs_configuration_desc;
    tusb_cfg.descriptor.string = string_desc_arr;
    tusb_cfg.descriptor.string_count = sizeof(string_desc_arr) / sizeof(string_desc_arr[0]);

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "tinyusb driver installed");

    return ESP_OK;
}