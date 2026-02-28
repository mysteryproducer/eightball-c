#include "esp_log.h"
#include <vector>
#include <string>
#include "tft.h"
#include "files.h"
#include "capi.h"
#include "gen.h"

using namespace EightBall;
using namespace std;

void *initGenerator(gen_config config) {
    //TextGenerator *gen = new TestGenerator();
    string filePath = string(FS_BASE) + "/"s + string(config.args);
    if (strncasecmp(config.gen_type, GEN_TYPE_FILE, 4) == 0) {
        return new LineReader(filePath.c_str());
    }
    if (strncasecmp(config.gen_type, GEN_TYPE_TEST, 4) == 0) {
        return new TestGenerator();
    }
    return new GrammarGenerator(filePath.c_str());
}

EightBallScreen *initScreen(lcd_config config) {
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
    return screen;
}

void screenPower(void *screenHandle, bool screenOn) {
    EightBallScreen *screen = (EightBallScreen *)screenHandle;
    screen->setScreenPower(screenOn);
}

void showString(void *screenHandle, const string &message) {
    EightBallScreen *screen = (EightBallScreen *)screenHandle;
    ESP_LOGI("main","New text: '%s'",message.c_str());
    screen->paintBackground();
    screen->drawText(message);
    screen->flush();
}
void showMessage(void *screenHandle, const char *message) {
    showString(screenHandle, string(message));
}
void newText(void *genHandle, void *screenHandle) {
    GrammarGenerator *gen = (GrammarGenerator *)genHandle;
    string text = gen->generateNext();
    showString(screenHandle, text);
}
