#include "esp_log.h"
#include <vector>
#include <string>
#include "tft.h"
#include "files.h"
#include "capi.h"
#include "gen.h"

using namespace EightBall;
using namespace std;

void * initScreen(lcd_config config) {
    esp_err_t result;
    EightBallScreen *screen = new EightBallScreen(config,&result);
    if (result!=ESP_OK) {
        delete screen;
        return NULL;
    }
    vector<string> fontFiles = {
        "/files/monaco14.bmf",
        "/files/monaco20.bmf",
        "/files/monaco28.bmf"
    };
    vector<Font *> *fonts = Font::loadFonts(&fontFiles);
    screen->loadFonts(fonts);
    return (void *)screen;
}

void screenPower(void *screenHandle, bool screenOn) {
    EightBallScreen *screen = (EightBallScreen *)screenHandle;
    screen->setScreenPower(screenOn);
}

void * initGenerator(gen_config config) {
    //TextGenerator *gen = new TestGenerator();
    string filePath = string(FS_BASE) + "/"s + string(config.args);
    GrammarGenerator *gen = new GrammarGenerator(filePath.c_str());
    return (void *)gen;
}

void newText(void *genHandle, void *screenHandle) {
    GrammarGenerator *gen = (GrammarGenerator *)genHandle;
    EightBallScreen *screen = (EightBallScreen *)screenHandle;
    string text = gen->generateNext();
    ESP_LOGI("main","New text: '%s'",text.c_str());
//    screen->initialiseBuffer();
    screen->paintBackground();
    screen->drawText(text);
    screen->flush();
}
