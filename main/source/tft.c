#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
//#include "esp_intr_types.h"
#include "esp_lcd_gc9a01.h"

#include "main.h"
#include "tft.h"
#include "pins.h"

static esp_lcd_panel_io_handle_t io_handle = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;
static SemaphoreHandle_t refresh_finish = NULL;

IRAM_ATTR static bool notify_refresh_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    //ESP_LOGI("gc9a01", "refresh ready");
    BaseType_t need_yield = pdFALSE;

    xSemaphoreGiveFromISR(refresh_finish, &need_yield);
    return (need_yield == pdTRUE);
}

esp_err_t setup_8ball_power_pin() {
    gpio_config_t cfg={
        .pin_bit_mask = 1 << PIN_TFT_POWER,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    esp_err_t result = gpio_config(&cfg);
    if (result!=ESP_OK) {
        ESP_LOGW(TAG, "Faled to configure TFT pin: GPIO %i",PIN_TFT_POWER);
    }
    return result;
}

esp_err_t set_8ball_tft_power(bool power_on) {
    return gpio_set_level(PIN_TFT_POWER,power_on?1:0);
}

esp_err_t setup_8ball_tft() {
    ESP_LOGI(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = 
        GC9A01_PANEL_BUS_SPI_CONFIG(PIN_SPI_CLK, PIN_SPI_MOSI,
                                    LCD_RES * LCD_RES * LCD_BIT_PER_PIXEL / 8);
    esp_err_t result = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (result!=ESP_OK) {
        return result;
    }
    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_spi_config_t io_config = 
        GC9A01_PANEL_IO_SPI_CONFIG(PIN_SPI_CS, PIN_SPI_DC,NULL, NULL);
//            .pclk_hz=800 * 1000,
    // Attach the LCD to the SPI bus
    result = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &io_handle);
    if (result!=ESP_OK) {
        return result;
    }
    ESP_LOGI(TAG, "Install gc9a01 panel driver");

    gc9a01_vendor_config_t vendor_cfg = {
        .init_cmds = gc9a01_init_cmds,
        .init_cmds_size = sizeof(gc9a01_init_cmds) / sizeof(gc9a01_init_cmds[0]),
    };
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_SPI_RESET,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = LCD_BIT_PER_PIXEL,
        .vendor_config = &vendor_cfg
    };
    result = esp_lcd_new_panel_gc9a01(io_handle, &panel_config, &panel_handle);
    if (result!=ESP_OK) { ESP_LOGI(TAG, "fail new"); }
    esp_lcd_panel_reset(panel_handle);
    if (result!=ESP_OK) { ESP_LOGI(TAG, "fail reset"); }
    esp_lcd_panel_init(panel_handle);
    if (result!=ESP_OK) { ESP_LOGI(TAG, "fail init"); }
    esp_lcd_panel_disp_on_off(panel_handle, true);
    if (result!=ESP_OK) { ESP_LOGI(TAG, "fail turn on"); }
    return result;
}

// #3120f5 = 0x311e
void draw_8ball_blank_blue() {
    refresh_finish = xSemaphoreCreateBinary();

    uint8_t byte_per_pixel = LCD_BIT_PER_PIXEL / 8;
    uint8_t bpl = byte_per_pixel * LCD_RES;
    uint8_t *color = (uint8_t *)heap_caps_calloc(1, LCD_RES * LCD_RES * byte_per_pixel, MALLOC_CAP_DMA);

    for (int j = 0; j < bpl; j+=2) {
        for (int i = 0; i < bpl; i+=2) {
            color[i * bpl + j] = 0x31;
            color[i * bpl + j+1] = 0x1e;
        }
    }
    esp_err_t res=esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_RES, LCD_RES, color);
    xSemaphoreTake(refresh_finish, portMAX_DELAY);
    free(color);
    vSemaphoreDelete(refresh_finish);
}