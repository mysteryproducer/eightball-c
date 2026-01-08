#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_gc9a01.h"

#include "gc9a01.h"
#include "tft.h"
#include <algorithm>

static const char *TAG = "8 ball TFT";

using namespace EightBall;

EightBallScreen::EightBallScreen(lcd_config config,esp_err_t *result) {
    this->powerPin = config.power_pin;
    this->width = config.width;
    this->height = config.height;
    *result = this->setupPowerPin();
    if (*result!=ESP_OK) {
        ESP_LOGW(TAG,"Error setting up LCD power pin.");
    } else {
        *result = setScreenPower(true);
    }
    *result = this->setupScreen(config);
}

esp_err_t EightBallScreen::setupPowerPin() {
    gpio_config_t cfg={
        .pin_bit_mask = (uint64_t)(1 << this->powerPin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    esp_err_t result = gpio_config(&cfg);
    if (result!=ESP_OK) {
        ESP_LOGW(TAG, "Faled to configure TFT pin: GPIO %i",this->powerPin);
    }
    return result;
}

esp_err_t EightBallScreen::setScreenPower(bool power_on) {
    esp_err_t result = gpio_set_level((gpio_num_t)this->powerPin,power_on?1:0);
    gpio_hold_en(this->powerPin);
    return result;
}

esp_err_t EightBallScreen::setupScreen(lcd_config config) {
    ESP_LOGI(TAG, "Initialize SPI bus");
    // Explicit bus config so we control max_transfer_sz (helps DMA/stride issues)
    const spi_bus_config_t buscfg = {
        .mosi_io_num = (int)config.mosi_pin,
        .miso_io_num = -1,
        .sclk_io_num = (int)config.clk_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = (int)(config.width * config.height * EightBallScreen::BYTES_PER_PIX),
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
    io_config.pclk_hz = config.freq_hz; // 20 MHz
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
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .data_endian = LCD_RGB_DATA_ENDIAN_BIG,
        .bits_per_pixel = (size_t)(8 * EightBallScreen::BYTES_PER_PIX),
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
    uint32_t whole_buffer = config.width * config.height * EightBallScreen::BYTES_PER_PIX;
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

// #3120f5 = 0x311e
esp_err_t EightBallScreen::redrawScreen(bool write) {
    size_t bpl = EightBallScreen::BYTES_PER_PIX * this->width;
    uint8_t *buf = screenBuffer;

    for (int j = 0; j < this->height; j++) {
        size_t row_offset = (size_t)j * bpl;
        for (size_t i = 0; i < bpl; i += 2) {
            size_t index = row_offset + i;
            buf[index] = backColour.high;
            buf[index + 1] = backColour.low;
        }
    }
    if (write) {
        return this->flush();
    }
    return ESP_OK;
}

esp_err_t EightBallScreen::flush() {
    esp_err_t res = esp_lcd_panel_draw_bitmap(this->panel_handle, 0, 0, this->width, this->height, this->screenBuffer);
    if (res != ESP_OK) {
        ESP_LOGW(TAG, "Draw fail: %d", res);
    } else {
        ESP_LOGI(TAG, "Draw ok");
    }
    return res;
}

esp_err_t EightBallScreen::loadFonts(vector<Font *> *fonts) {
    this->fonts = fonts;
    return ESP_OK;
}

size_t EightBallScreen::getWidth() {
    return this->width;
}
size_t EightBallScreen::getHeight() {
    return this->height;
}
