#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_gc9a01.h"
#include "esp_log.h"
#include "screen.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BYTES_PER_PIX 2

const char *TAG = "8 ball TFT test";
static const uint8_t powerPin = 7;//21;

static esp_lcd_panel_io_handle_t io_handle = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;
static uint8_t *screenBuffer = NULL;

typedef struct {
    uint16_t clk_pin;
    uint16_t mosi_pin;
    uint16_t cs_pin;
    uint16_t dc_pin;
    uint16_t reset_pin;
    uint16_t width;
    uint16_t height;
} lcd_config;
lcd_config config = {
    .clk_pin = 0,
    .mosi_pin = 1,
    .dc_pin = 2,
    .cs_pin = 3,
    .reset_pin = 4,
    .width = 240,
    .height = 240
};
typedef struct {
    uint8_t high;
    uint8_t low;
} Colour565;
//static const Colour565 foreColour = {.high=0xff,.low=0xff};
// Use an explicit RGB565 blue (0x001F). Swap bytes if your panel expects little-endian data.
static const Colour565 backColour = {.high=0x31, .low=0x1E};

// Custom GC9A01 init sequence
static const gc9a01_lcd_init_cmd_t gc9a01_init_cmds[] = {
    {0xEF, NULL, 0, 0},
    {0xEB, (uint8_t[]){0x14}, 1, 0},
    {0xFE, NULL, 0, 0},
    {0xEF, NULL, 0, 0},
    {0xEB, (uint8_t[]){0x14}, 1, 0},

    {0x84, (uint8_t[]){0x40}, 1, 0},
    {0x85, (uint8_t[]){0xFF}, 1, 0},
    {0x86, (uint8_t[]){0xFF}, 1, 0},
    {0x87, (uint8_t[]){0xFF}, 1, 0},
    {0x88, (uint8_t[]){0x0A}, 1, 0},
    {0x89, (uint8_t[]){0x21}, 1, 0},
    {0x8A, (uint8_t[]){0x00}, 1, 0},
    {0x8B, (uint8_t[]){0x80}, 1, 0},
    {0x8C, (uint8_t[]){0x01}, 1, 0},
    {0x8D, (uint8_t[]){0x01}, 1, 0},
    {0x8E, (uint8_t[]){0xFF}, 1, 0},
    {0x8F, (uint8_t[]){0xFF}, 1, 0},

    {0xB6, (uint8_t[]){0x00}, 1, 0},

    {0x36, (uint8_t[]){0x08}, 1, 0},   // Memory Access Control
    {0x3A, (uint8_t[]){0x05}, 1, 0},   // Pixel Format: RGB565

    {0x90, (uint8_t[]){0x08,0x08,0x08,0x08}, 4, 0},
    {0xBD, (uint8_t[]){0x06}, 1, 0},
    {0xBC, (uint8_t[]){0x00}, 1, 0},
    {0xFF, (uint8_t[]){0x60,0x01,0x04}, 3, 0},
    {0xC3, (uint8_t[]){0x13}, 1, 0},
    {0xC4, (uint8_t[]){0x13}, 1, 0},
    {0xC9, (uint8_t[]){0x22}, 1, 0},
    {0xBE, (uint8_t[]){0x11}, 1, 0},
    {0xE1, (uint8_t[]){0x10,0x0E}, 2, 0},
    {0xDF, (uint8_t[]){0x21,0x0C,0x02}, 3, 0},
    {0xF0, (uint8_t[]){0x45,0x09,0x08,0x08,0x26,0x2A}, 6, 0},
    {0xF1, (uint8_t[]){0x43,0x70,0x72,0x36,0x37,0x6F}, 6, 0},
    {0xF2, (uint8_t[]){0x45,0x09,0x08,0x08,0x26,0x2A}, 6, 0},
    {0xF3, (uint8_t[]){0x43,0x70,0x72,0x36,0x37,0x6F}, 6, 0},
    {0xED, (uint8_t[]){0x1B,0x0B}, 2, 0},
    {0xAE, (uint8_t[]){0x77}, 1, 0},
    {0xCD, (uint8_t[]){0x63}, 1, 0},

    {0x70, (uint8_t[]){0x07,0x07,0x04,0x0E,0x0F,0x09,0x07,0x08,0x03}, 9, 0},

    {0xE8, (uint8_t[]){0x34}, 1, 0},
    {0x62, (uint8_t[]){0x18,0x0D,0x71,0xED,0x70,0x70,0x18,0x0F,0x71,0xEF,0x70,0x70}, 12, 0},
    {0x63, (uint8_t[]){0x18,0x11,0x71,0xF1,0x70,0x70,0x18,0x13,0x71,0xF3,0x70,0x70}, 12, 0},
    {0x64, (uint8_t[]){0x28,0x29,0xF1,0x01,0xF1,0x00,0x07}, 7, 0},
    {0x66, (uint8_t[]){0x3C,0x00,0xCD,0x67,0x45,0x45,0x10,0x00,0x00,0x00}, 10, 0},
    {0x67, (uint8_t[]){0x00,0x3C,0x00,0x00,0x00,0x01,0x54,0x10,0x32,0x98}, 10, 0},
    {0x74, (uint8_t[]){0x10,0x85,0x80,0x00,0x00,0x4E,0x00}, 7, 0},
    {0x98, (uint8_t[]){0x3E,0x07}, 2, 0},

    {0x35, NULL, 0, 0},   // TE ON
    {0x21, NULL, 0, 0},   // Inversion ON
    {0x11, NULL, 0, 120}, // Sleep Out + delay
    {0x29, NULL, 0, 0},   // Display ON
};

esp_err_t redrawScreen(bool write);
/* Production build: diagnostic helpers removed. */

esp_err_t setupPowerPin(/*uint8_t powerPin*/) {
    gpio_config_t cfg={
        .pin_bit_mask = (uint64_t)(1 << powerPin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    esp_err_t result = gpio_config(&cfg);
    if (result!=ESP_OK) {
        ESP_LOGW(TAG, "Faled to configure TFT pin: GPIO %i",powerPin);
    }
    return result;
}

esp_err_t setScreenPower(bool power_on) {
    return gpio_set_level((gpio_num_t)powerPin,power_on?1:0);
}

esp_err_t init_screen() {
    ESP_LOGI(TAG, "Initialize SPI bus");
    // Explicit bus config so we control max_transfer_sz (helps DMA/stride issues)
    const spi_bus_config_t buscfg = {
        .mosi_io_num = (int)config.mosi_pin,
        .miso_io_num = -1,
        .sclk_io_num = (int)config.clk_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = (int)(config.width * config.height * BYTES_PER_PIX),
    };
    esp_err_t result = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (result!=ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus");
        return result;
    }
    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_spi_config_t io_config = 
        GC9A01_PANEL_IO_SPI_CONFIG(config.cs_pin, config.dc_pin, NULL, NULL);
    // Lower panel SPI clock to reduce signal integrity issues visible as vertical artifacts
    io_config.pclk_hz = 8000000; // 8 MHz
    // Attach the LCD to the SPI bus
    result = esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &io_handle);
    if (result!=ESP_OK) {
        ESP_LOGE(TAG, "Failed to install panel IO");
        return result;
    }
    ESP_LOGI(TAG, "Install gc9a01 panel driver");

    gc9a01_vendor_config_t vendor_cfg = {
        .init_cmds = gc9a01_init_cmds,
        .init_cmds_size = sizeof(gc9a01_init_cmds) / sizeof(gc9a01_init_cmds[0]),
    };
    // Final working configuration: big-endian, RGB element order
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = config.reset_pin,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .data_endian = LCD_RGB_DATA_ENDIAN_BIG,
        .bits_per_pixel = (size_t)(8 * BYTES_PER_PIX),
        .flags = {},
        .vendor_config = &vendor_cfg
    };
    result = esp_lcd_new_panel_gc9a01(io_handle, &panel_config, &panel_handle);
    if (result!=ESP_OK) { ESP_LOGE(TAG, "fail new screen"); }
    esp_lcd_panel_reset(panel_handle);
    if (result!=ESP_OK) { ESP_LOGW(TAG, "fail reset screen"); }
    esp_lcd_panel_init(panel_handle);
    if (result!=ESP_OK) { ESP_LOGW(TAG, "fail init screen"); }
    esp_lcd_panel_disp_on_off(panel_handle, true);
    if (result!=ESP_OK) { ESP_LOGW(TAG, "fail turn on screen"); }

    //this->byte_per_pixel = LCD_BIT_PER_PIXEL / 8;
    uint32_t whole_buffer = config.width * config.height * BYTES_PER_PIX;
    screenBuffer = (uint8_t *)heap_caps_calloc(1, whole_buffer, MALLOC_CAP_DMA);
    if (screenBuffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate screen buffer (%u bytes)", whole_buffer);
        return ESP_ERR_NO_MEM;
    }

    if (result == ESP_OK) {
        result = setScreenPower(true);
        ESP_LOGI(TAG, "Screen initialized. Drawing background.");
        redrawScreen(true);
    } else {
        ESP_LOGE(TAG, "Failed to initialize screen");
    }
    return result;
}

esp_err_t redrawScreen(bool write) {
    //try {
//        this->semaphore = xSemaphoreCreateBinary();
//        int whole_buffer = this->width * this->height * EightBallScreen::BYTES_PER_PIX;
//        uint8_t *buf =  (uint8_t *)heap_caps_calloc(1, whole_buffer, MALLOC_CAP_DMA);
        size_t bpl = (size_t)BYTES_PER_PIX * config.width;
        uint8_t *buf = screenBuffer;

        for (int j = 0; j < config.height; j++) {
            size_t row_offset = (size_t)j * bpl;
            for (size_t i = 0; i < bpl; i += 2) {
                size_t index = row_offset + i;
                buf[index] = backColour.high;
                buf[index + 1] = backColour.low;
            }
        }
        if (write) {
            esp_err_t res = esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, config.width, config.height, buf);
            if (res != ESP_OK) {
                ESP_LOGW(TAG, "Draw fail: %d", res);
            } else {
                ESP_LOGI(TAG, "Draw ok");
            }
        }
//        xSemaphoreTake(this->semaphore, portMAX_DELAY);
//        free(buf);
//        vSemaphoreDelete(this->semaphore);
    //} catch (const std::exception& e) {
    //    ESP_LOGE(TAG,"Error repainting screen: %s" + e);
    //}
    return ESP_OK;
}

void blue_screen() {
    esp_err_t rs = setupPowerPin();
    if (rs != ESP_OK) {
        ESP_LOGW(TAG, "setupPowerPin failed: %d", rs);
    }
    esp_err_t rc = setScreenPower(true);
    if (rc != ESP_OK) { ESP_LOGW(TAG, "setScreenPower failed: %d", rc); }
    rc = init_screen();
    if (rc != ESP_OK) { ESP_LOGW(TAG, "init_screen failed: %d", rc); }
    redrawScreen(true);
}

/* Diagnostics removed; keeping init and draw code minimal for production.
   If you need diagnostics again, they are available in earlier commits. */

// Draw alternating vertical stripes using two RGB565 colors (high/low bytes provided)
static void draw_vertical_stripes(uint8_t high1, uint8_t low1, uint8_t high2, uint8_t low2, int stripe_width) {
    if (screenBuffer == NULL) return;
    for (int x = 0; x < config.width; x++) {
        int which = (x / stripe_width) & 1;
        uint8_t h = which ? high2 : high1;
        uint8_t l = which ? low2 : low1;
        for (int y = 0; y < config.height; y++) {
            size_t idx = (size_t)y * (size_t)BYTES_PER_PIX * config.width + (size_t)x * BYTES_PER_PIX;
            screenBuffer[idx] = h;
            screenBuffer[idx + 1] = l;
        }
    }
}

// Recreate panel with specified endian and rgb order. Returns ESP_OK on success.
static esp_err_t recreate_panel(bool big_endian, bool rgb_order_rgb) {
    if (panel_handle) {
        esp_lcd_panel_del(panel_handle);
        panel_handle = NULL;
    }
    gc9a01_vendor_config_t vendor_cfg = {
        .init_cmds = gc9a01_init_cmds,
        .init_cmds_size = sizeof(gc9a01_init_cmds) / sizeof(gc9a01_init_cmds[0]),
    };
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = config.reset_pin,
        .rgb_ele_order = rgb_order_rgb ? LCD_RGB_ELEMENT_ORDER_RGB : LCD_RGB_ELEMENT_ORDER_BGR,
        .data_endian = big_endian ? LCD_RGB_DATA_ENDIAN_BIG : LCD_RGB_DATA_ENDIAN_LITTLE,
        .bits_per_pixel = (size_t)(8 * BYTES_PER_PIX),
        .flags = {},
        .vendor_config = &vendor_cfg
    };
    esp_err_t r = esp_lcd_new_panel_gc9a01(io_handle, &panel_config, &panel_handle);
    if (r != ESP_OK) {
        ESP_LOGW(TAG, "recreate_panel failed (%d) endian=%d rgb_rgb=%d", r, big_endian, rgb_order_rgb);
        return r;
    }
    esp_lcd_panel_reset(panel_handle);
    vTaskDelay(pdMS_TO_TICKS(50));
    esp_lcd_panel_init(panel_handle);
    vTaskDelay(pdMS_TO_TICKS(50));
    esp_lcd_panel_disp_on_off(panel_handle, true);
    return ESP_OK;
}

// Try combinations of endianness and rgb order and draw vertical stripes for visual inspection.
static void test_endian_and_order(void) {
    ESP_LOGI(TAG, "Testing endian and RGB order combinations");
    const int stripe_w = 8;
    // Colors: blue (0x001F) and red (0xF800) in RGB565
    uint8_t blue_h = 0x00, blue_l = 0x1F;
    uint8_t red_h = 0xF8, red_l = 0x00;
    bool combos[4][2] = {{true,true},{true,false},{false,true},{false,false}}; // {big_endian, rgb_rgb}
    for (int i = 0; i < 4; i++) {
        bool big = combos[i][0];
        bool rgb = combos[i][1];
        ESP_LOGI(TAG, "Recreate panel: big_endian=%d rgb_rgb=%d", big, rgb);
        if (recreate_panel(big, rgb) != ESP_OK) continue;
        draw_vertical_stripes(blue_h, blue_l, red_h, red_l, stripe_w);
        esp_err_t r = esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, config.width, config.height, screenBuffer);
        ESP_LOGI(TAG, "Draw stripes (endian=%d rgb=%d) -> %d", big, rgb, r);
        vTaskDelay(pdMS_TO_TICKS(1200));
    }
    // Recreate original config (big endian, RGB) to restore behavior
    recreate_panel(true, true);
}
