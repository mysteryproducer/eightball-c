#pragma once
#include "main.h"
#include "gen.h"
#include "esp_lcd_gc9a01.h"
#include "driver/spi_master.h"
#include <string>
#include <map>
#include <vector>

using namespace std;

namespace EightBall {

typedef struct {
    int xPos;
    int yPos;
    string line;
} DisplayLine;

typedef struct {
    float screen_diameter;// =self.tft.height - 10
    float radius;// = screen_diameter / 2
    size_t center_y;// = radius

    size_t max_lines;
    size_t start_y;
    vector<size_t> *positions;
} FontMetrics;

typedef struct {
    size_t x;
    size_t y;
    size_t width;
    size_t height;
} Rectangle;

typedef struct {
    uint8_t high;
    uint8_t low;
} Colour565;

class EightBallScreen;

class Font {
    public:
        Font(const char *filePath);
        uint8_t getWidth();
        uint8_t getHeight();
        uint8_t *write(string text,const Colour565 *foreColour,const Colour565 *backColour);
        void writeText(uint8_t *buffer, EightBallScreen *screen, vector<DisplayLine *> *layout);
        void writeTo(uint8_t *buffer, size_t width, size_t height, 
            string text, int x, int y, 
            const Colour565 *foreColour, const Colour565 *backColour);
    protected:
        Font();
        uint8_t width;
        uint8_t height;
        map<uint8_t,uint8_t*> bitmaps;
        int bytesPerChar;
    private:
        bool loadFont(const char *path);
};

class EightBallScreen {
    public:
        const int BYTES_PER_PIX=2;
        constexpr static const Colour565 foreColour = {.high=0xff,.low=0xff};
        constexpr static const Colour565 backColour = {.high=0x31,.low=0x1e};

        EightBallScreen(lcd_config config,esp_err_t *result);
        esp_err_t setScreenPower(bool screenOn);
        esp_err_t redrawScreen(bool write=true);
        esp_err_t drawText(string text);
        esp_err_t loadFonts(vector<string> files);
        size_t getWidth();
        size_t getHeight();
        unsigned short background();
        unsigned short foreground();

    private:
        SemaphoreHandle_t semaphore = NULL;
        esp_lcd_panel_io_handle_t io_handle = NULL;
        esp_lcd_panel_handle_t panel_handle = NULL;
        uint8_t powerPin;
        esp_err_t setupScreen(lcd_config config);
        esp_err_t setupPowerPin();

        vector<Font *> fonts;
        size_t width;
        size_t height;
        uint8_t *screenBuffer;
        Rectangle lastDrawBounds;

        map<int,FontMetrics *> metrics;
        vector<DisplayLine*> *layoutText(string displayText);
        FontMetrics *getLinePositions(size_t font_height);
        vector<DisplayLine *> *layoutTextInCircle(vector<string> *words,Font *font);
};

};

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

