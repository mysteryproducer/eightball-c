#include "esp_log.h"
#include <vector>
#include <string>
#include "tft.h"
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

void * initGenerator() {
    TextGenerator *gen = new TestGenerator();
    //GrammarGenerator *gen = new GrammarGenerator("/files/dsm5.txt");
    return (void *)gen;
}

void newText(void *genHandle, void *screenHandle) {
    GrammarGenerator *gen = (GrammarGenerator *)genHandle;
    EightBallScreen *screen = (EightBallScreen *)screenHandle;
    string text = gen->generateNext();
    ESP_LOGI("main","New text: %s",text.c_str());
    screen->beginPainting();
    screen->paintBackground();
    screen->drawText(text);
    screen->flush();
}
