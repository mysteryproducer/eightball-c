#include <errno.h>
#include <dirent.h>
#include "driver/gpio.h"
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
//#define EPNUM_MSC       1
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

static void light_it_up(uint8_t level=1, uint8_t pin=7) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level((gpio_num_t)pin, level);
}

/**
 * @brief Storage mount changed callback
 *
 * @param handle Storage handle
 * @param event Event information
 * @param arg User argument, provided during callback registration
 */
static void storage_mount_changed_cb(tinyusb_event_t *event, void *arg)
{
    if (event->id == TINYUSB_EVENT_ATTACHED) {
        //light_it_up(1,3);
        mount_callback();
    } else {//if (event->id == TINYUSB_EVENT_DETACHED) {
        light_it_up(1,7);
        unmount_callback();
    }
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

    esp_err_t err = tinyusb_msc_new_storage_spiflash(&storage_cfg, &msc_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize USB MSC storage: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "MSC storage created (handle=%p)", msc_handle);

    tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    tusb_cfg.descriptor.device = &descriptor_config;
    tusb_cfg.descriptor.full_speed_config = msc_fs_configuration_desc;
    tusb_cfg.descriptor.string = string_desc_arr;
    tusb_cfg.descriptor.string_count = sizeof(string_desc_arr) / sizeof(string_desc_arr[0]);
    tusb_cfg.event_cb = storage_mount_changed_cb;
    tusb_cfg.event_arg = NULL;

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "tinyusb driver installed");

    return ESP_OK;
}

esp_err_t stop_usb_msc() {
    esp_err_t err = tinyusb_driver_uninstall();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to uninstall tinyusb driver: %s", esp_err_to_name(err));
        return err;
    }
    if (msc_handle) {
        err = tinyusb_msc_delete_storage(msc_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to delete MSC storage: %s", esp_err_to_name(err));
            return err;
        }
        msc_handle = NULL;
    }
    if (wl_handle != WL_INVALID_HANDLE) {
        err = wl_unmount(wl_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to unmount wear levelling: %s", esp_err_to_name(err));
            return err;
        }
        wl_handle = WL_INVALID_HANDLE;
    }
    return err;
}