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
    esp_err_t returnCode=init_filesystem();
    if (returnCode != ESP_OK) {
        ESP_LOGE(TAG,"Couldn't init filesystem. Failed to load %s. Error code %i",path,returnCode);
    }
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
    close_filesystem();
    return result;
}

void Font::writeText(uint8_t *buffer, EightBallScreen *screen, vector<DisplayLine *> *layout) {
    size_t width=screen->getWidth();
    size_t height=screen->getHeight();
    for(DisplayLine *line : *layout) {
        this->writeTo(buffer,width,height,
            line->line,line->xPos,line->yPos,
            &(EightBallScreen::foreColour),&(EightBallScreen::backColour));
    }
}

void Font::writeTo(uint8_t *buffer, size_t width, size_t height, string text, int x, int y, const Colour565 *foreColour, const Colour565 *backColour) {
    //flipping the x-axis; start at the end and work backward.
    x+=this->width * text.length();
    for (int i=0;i<text.length();++i) {
        char letter = text[i];
        if (letter != ' ') {
//            auto ptr = charBins.find(letter);
            uint8_t *charBinary;
//            if (ptr == charBins.end()) {
                //ESP_LOGW(TAG, "No bitmap for character '%c' (code %i)", letter, (int)letter);
                charBinary = createLetter(letter, foreColour, backColour);
//                charBins[letter] = charBinary;
//            } else {
//                charBinary = ptr->second;
//            }
            for (size_t row=0;row<this->height;++row) {
                for (size_t col=0;col<this->width;++col) {
                    size_t screen_x = x - (i+1)*this->width + col;
                    size_t screen_y = y + row;
                    if (screen_x >= width || screen_y >= height) {
                        continue;
                    }
                    size_t screen_index = (screen_y * width + screen_x) * 2;
                    size_t char_index = (row * this->width + col) * 2;
                    buffer[screen_index] = charBinary[char_index];
                    buffer[screen_index + 1] = charBinary[char_index + 1];
                }
            }
            free(charBinary);
        }
    }
}

uint8_t *Font::createLetter(char character, const Colour565 *foreColour, const Colour565 *backColour) {
//    uint8_t masks[] = uint8_t[0x7F,0xBF,0xDF,0xEF,0xF7,0xFB,0xFD,0xFE];
    size_t pixels = this->width*this->height; 
    size_t bufsize = pixels*2;
    uint8_t *buffer = (uint8_t *)malloc(bufsize);//heap_caps_calloc(bufsize,1,MALLOC_CAP_DMA);
    if (buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for character bitmap");
        return NULL;
    }
    auto search = this->bitmaps.find(character);
    if (search == this->bitmaps.end()) {
        for (size_t i=0;i<bufsize;i+=2) {
            buffer[i] = backColour->high;
            buffer[i+1] = backColour->low;
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
                size_t buffer_index = ((row+1) * this->width - col) * 2;
                bool pixel_value = (byte & bit_mask) != 0;
                bit_mask >>= 1;
                if (pixel_value) {
                    buffer[buffer_index] = foreColour->high;
                    buffer[buffer_index + 1] = foreColour->low;
                } else {
                    buffer[buffer_index] = backColour->high;
                    buffer[buffer_index + 1] = backColour->low;
                }
            }
        }
    }
    return buffer;
}
