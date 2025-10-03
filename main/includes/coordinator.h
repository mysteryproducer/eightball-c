#pragma once
#include <string>
#include "esp_system.h"
#include "gen.h"
#include "tft.h"
#include "accelerometer.h"

namespace EightBall {

    class EightBallCoordinator {
        public:
            EightBallCoordinator();
            esp_err_t initScreen(lcd_config config);
            esp_err_t initAccelerometer(mpu6050_cfg config);
            esp_err_t initGenerator(const char *type, const char *arg);
            esp_err_t loadFonts(vector<string> files);
            TextGenerator *getGenerator();
            Accelerometer *getAccelerometer();
            EightBallScreen *getScreen();
        private:
            TextGenerator *generator;
            Accelerometer *accelerometer;
            EightBallScreen *screen;
    };

    EightBallCoordinator eightBall;
}