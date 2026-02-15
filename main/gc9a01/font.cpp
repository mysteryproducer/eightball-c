#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <set>
#include "esp_log.h"
#include "files.h"
#include "tft.h"
#include "math.h"

using namespace std;
using namespace EightBall;

static const char *TAG = "8 ball Font";

vector<Font *> *EightBall::Font::loadFonts(vector<string> *files) {
    vector<Font *> *result = new vector<Font *>();
    if (init_filesystem() != ESP_OK) {
        ESP_LOGE(TAG,"File system setup fail!");
        return NULL;
    }
    for (int i=0;i<files->size();++i) {
        Font *f = new Font((*files)[i].c_str());
        result->push_back(f);
    }
    sort(result->begin(),result->end());
    reverse(result->begin(),result->end());
    close_filesystem();
    return result;
}

Font::Font(const char *filePath) {
    this->loadFont(filePath);
}
uint8_t Font::getWidth() {
    return this->width;
}
uint8_t Font::getHeight() {
    return this->height;
}
bool Font::loadFont(const char *path) {
    ESP_LOGI(TAG,"loading font %s",path);
    bool result=false;
    ifstream infile(path,ios::in|ios::binary);
    char *sz_byte=new char[1];
    if (infile.is_open()) {
        infile.read(sz_byte,1);
        this->width = sz_byte[0];
        infile.read(sz_byte,1);
        this->height = sz_byte[0];

        this->bytesPerChar=this->width*this->height/8;
        while (!infile.eof()) {
            char *letterMap = (char *)malloc(this->bytesPerChar);
            infile.read(sz_byte,1);
            if (infile.eof()) {
                break;
            }
            uint8_t ord = sz_byte[0];
            infile.read(letterMap, this->bytesPerChar);
            if (infile.gcount() < this->bytesPerChar) {
                ESP_LOGW(TAG,"Bad file alignment. Failed to load letter %i from %s. Requires %i bytes; %i available.",ord,path,this->bytesPerChar,infile.gcount());
            }
            this->bitmaps[ord]=reinterpret_cast<uint8_t *>(letterMap);
        }
    } else {
        ESP_LOGE(TAG,"Couldn't open file: %s",path);
    }
    delete[] sz_byte;
    return result;
}

void Font::writeText(uint8_t *buffer, size_t width, size_t height, const vector<DisplayLine> &layout, uint16_t foreColour, uint16_t backColour) {
    for(DisplayLine line : layout) {
        this->writeTo(buffer,width,height,
            line.line.c_str(),line.xPos,line.yPos,
            foreColour,backColour);
    }
}

void Font::writeTo(uint8_t *buffer, size_t width, size_t height, const char *text, int x, int y, uint16_t foreColour, uint16_t backColour) {
    //flipping the x-axis; start at the end and work backward.
    x+=this->width * strlen(text);
    //todo: is it worth optimising here? Could cache letter bitmaps and/or use memcpy to move from letter buffer to screen buffer.
//    map<char,uint8_t *> letterCache;
    for (int i=0;i<strlen(text);++i) {
        char letter = text[i];
        if (letter != ' ') {
            size_t charBufSize=this->width * this->height * 2;
            uint8_t charBinary[charBufSize];
            createLetter(letter, foreColour, backColour, charBinary);
            for (size_t row=0;row<this->height;++row) {
                for (size_t col=0;col<this->width;++col) {
                    size_t screen_x = x - i*this->width - col;
                    size_t screen_y = y + row;
                    if (screen_x >= width || screen_y >= height) {
                        continue;
                    }
                    size_t screen_index = (screen_y * width + screen_x) * 2;
                    size_t char_index = (row * this->width + col) * 2;
                    if (screen_index + 1 >= width * height * 2 || 
                        char_index + 1 >= width * height * 2) {
                        ESP_LOGE(TAG,"Index out of bounds: screen_index %i, char_index %i",screen_index,char_index);
                        continue;
                    }
                    buffer[screen_index] = charBinary[char_index];
                    buffer[screen_index + 1] = charBinary[char_index + 1];
                }
            }
        }
    }
}

bool Font::createLetter(char character, uint16_t foreColour, uint16_t backColour, uint8_t *buffer) {
    size_t pixels = this->width*this->height;
    size_t bufsize = pixels*2;
    auto search = this->bitmaps.find(character);
    if (search == this->bitmaps.end()) {
        for (size_t i=0;i<bufsize;i+=2) {
            buffer[i] = backColour >> 8;
            buffer[i+1] = backColour & 0xFF;
        }
    } else {
        uint8_t byteCount = pixels / 8;
        for (size_t i=0;i<byteCount;++i) {
            uint8_t byte = search->second[i];
            uint8_t bit_mask = 0x80;
            for (size_t bit=0;bit<8;++bit) {
                size_t pixel_index = i*8 + bit;
                size_t row = pixel_index / this->width;
                size_t col = pixel_index % this->width;
                size_t buffer_index = (row * this->width + col) * 2;
                bool pixel_value = (byte & bit_mask) != 0;
                bit_mask >>= 1;
                if (pixel_value) {
                    buffer[buffer_index] = foreColour >> 8;
                    buffer[buffer_index + 1] = foreColour & 0xFF;
                } else {
                    buffer[buffer_index] = backColour >> 8;
                    buffer[buffer_index + 1] = backColour & 0xFF;
                }
            }
        }
    }
    return true;
}
