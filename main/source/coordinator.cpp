#include <string>
#include "coordinator.h"

using namespace std;
using namespace EightBall;

EightBallCoordinator::EightBallCoordinator() {}

esp_err_t EightBallCoordinator::initScreen(lcd_config config) {
    esp_err_t result;
    this->screen = new EightBallScreen(config,&result);
    return result;
}

esp_err_t EightBallCoordinator::initAccelerometer(mpu6050_cfg config) {
    esp_err_t result;
    this->accelerometer = new Accelerometer(config,&result);
    return result;
}

esp_err_t EightBallCoordinator::initGenerator(const char *type, const char *arg) {
    this->generator = new GrammarGenerator(arg);
    return ESP_OK;
}

esp_err_t EightBallCoordinator::loadFonts(vector<string> files) {
    return this->getScreen()->loadFonts(files);
}

Accelerometer *EightBallCoordinator::getAccelerometer() {
    return this->accelerometer;
}
TextGenerator *EightBallCoordinator::getGenerator() {
    return this->generator;
}
EightBallScreen *EightBallCoordinator::getScreen() {
    return this->screen;
}

extern "C" {

// esp_err_t draw_blank_blue() {
//     return eightBall.getScreen()->redrawScreen();
// }

// esp_err_t set_screen_power(bool screenOn) {
//     return eightBall.getScreen()->setScreenPower(screenOn);
// }

esp_err_t init_screen(lcd_config config) {
    return eightBall.initScreen(config);
}

uint8_t get_mpu6050_id() {
    return eightBall.getAccelerometer()->getDeviceID();
}

esp_err_t setup_8ball_mpu6050(mpu6050_cfg config) {
    return eightBall.initAccelerometer(config);
}

esp_err_t init_generator(const char *filename) {
    return eightBall.initGenerator(NULL,filename);
}

esp_err_t load_fonts(const char *filenames[],size_t num_files) {
    vector<string> pathVector;
    for (int i=0;i<num_files;++i) {
        pathVector.push_back(filenames[i]);
    }
    return eightBall.getScreen()->loadFonts(pathVector);
}

void on_timer(void *args) {
    double g_load=eightBall.getAccelerometer()->getGLoad();
    if (g_load>1.3) {
//        ESP_LOGI("timer", "%f", g_load);
        TextGenerator *gen=eightBall.getGenerator();
        string text=gen->generateNext();
        ESP_LOGI("timer","%s", text.c_str());
        eightBall.getScreen()->redrawScreen(false);
        eightBall.getScreen()->drawText(text);
    }
}

}