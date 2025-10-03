#include "main.h"
#include "tft.h"

esp_err_t draw_blank_blue() {
    return screen->redrawScreen();
}

esp_err_t set_screen_power(bool screenOn) {
    return screen->setScreenPower(screenOn);
}

esp_err_t init_screen(lcd_config config) {
    screen = new EightBallScreen(config);
    return ESP_OK;
}