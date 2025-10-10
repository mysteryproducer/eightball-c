#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_gc9a01.h"

#include "main.h"
#include "tft.h"
#include <algorithm>

using namespace EightBall;
// IRAM_ATTR static bool notify_refresh_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
// {
//     //ESP_LOGI("gc9a01", "refresh ready");
//     BaseType_t need_yield = pdFALSE;

//     xSemaphoreGiveFromISR(refresh_finish, &need_yield);
//     return (need_yield == pdTRUE);
// }

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
    return gpio_set_level((gpio_num_t)this->powerPin,power_on?1:0);
}

esp_err_t EightBallScreen::setupScreen(lcd_config config) {
    ESP_LOGI(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = 
        GC9A01_PANEL_BUS_SPI_CONFIG(config.clk_pin, config.mosi_pin,
                                    config.width * config.height * EightBallScreen::BYTES_PER_PIX);
    esp_err_t result = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (result!=ESP_OK) {
        return result;
    }
    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_spi_config_t io_config = 
        GC9A01_PANEL_IO_SPI_CONFIG(config.cs_pin, config.dc_pin, NULL, NULL);
//            .pclk_hz=800 * 1000,
    // Attach the LCD to the SPI bus
    result = esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &this->io_handle);
    if (result!=ESP_OK) {
        return result;
    }
    ESP_LOGI(TAG, "Install gc9a01 panel driver");

    gc9a01_vendor_config_t vendor_cfg = {
        .init_cmds = gc9a01_init_cmds,
        .init_cmds_size = sizeof(gc9a01_init_cmds) / sizeof(gc9a01_init_cmds[0]),
    };
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = config.reset_pin,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = (size_t)(8 * EightBallScreen::BYTES_PER_PIX),
        .vendor_config = &vendor_cfg
    };
    result = esp_lcd_new_panel_gc9a01(io_handle, &panel_config, &this->panel_handle);
    if (result!=ESP_OK) { ESP_LOGE(TAG, "fail new screen"); }
    esp_lcd_panel_reset(panel_handle);
    if (result!=ESP_OK) { ESP_LOGW(TAG, "fail reset screen"); }
    esp_lcd_panel_init(panel_handle);
    if (result!=ESP_OK) { ESP_LOGW(TAG, "fail init screen"); }
    esp_lcd_panel_disp_on_off(panel_handle, true);
    if (result!=ESP_OK) { ESP_LOGW(TAG, "fail turn on screen"); }

    //this->byte_per_pixel = LCD_BIT_PER_PIXEL / 8;
    uint32_t whole_buffer = this->width * this->height * EightBallScreen::BYTES_PER_PIX;
    this->screenBuffer = (uint8_t *)heap_caps_calloc(1, whole_buffer, MALLOC_CAP_DMA);

    if (result == ESP_OK) {
        result = setScreenPower(true);
        this->redrawScreen();
    }
    return result;
}

// #3120f5 = 0x311e
esp_err_t EightBallScreen::redrawScreen(bool write) {
    //try {
//        this->semaphore = xSemaphoreCreateBinary();
//        int whole_buffer = this->width * this->height * EightBallScreen::BYTES_PER_PIX;
//        uint8_t *buf =  (uint8_t *)heap_caps_calloc(1, whole_buffer, MALLOC_CAP_DMA);
        int bpl = EightBallScreen::BYTES_PER_PIX * this->width;
        uint8_t *buf = this->screenBuffer;

        for (int j = 0; j < this->height; j++) {
            for (int i = 0; i < bpl; i+=2) {
                int index = bpl * j + i;
                buf[index] = EightBallScreen::backColour.high;
                buf[index+1] = EightBallScreen::backColour.low;
            }
        }
        if (write) {
//            ESP_LOGI(TAG,"drawing %i x %i pixels from %i, %i",this->width,this->height,0,0);
            esp_err_t res=esp_lcd_panel_draw_bitmap(this->panel_handle, 0, 0, this->width, this->height, buf);
            if (res!=ESP_OK) {
                ESP_LOGW(TAG, "Draw fail!");
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

esp_err_t EightBallScreen::loadFonts(vector<string> files) {
    if (init_filesystem() != ESP_OK) {
        ESP_LOGE(TAG,"File system setup fail!");
        return ESP_ERR_INVALID_STATE;
    }
    for (int i=0;i<files.size();++i) {
        Font *f = new Font(files[i].c_str());
        this->fonts.push_back(f);
    }
    sort(this->fonts.begin(),this->fonts.end());
    reverse(this->fonts.begin(),this->fonts.end());
    close_filesystem();
    return ESP_OK;
}

size_t EightBallScreen::getWidth() {
    return this->width;
}
size_t EightBallScreen::getHeight() {
    return this->height;
}