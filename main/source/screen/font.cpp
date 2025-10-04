#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include "main.h"
#include "tft.h"
#include "math.h"

using namespace std;
using namespace EightBall;
    
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
uint8_t *Font::write(string text,const Colour565 *foreColour,const Colour565 *backColour) {
    size_t bmWidth=this->width*text.length();
    size_t bufferSize=bmWidth * this->height;
    uint8_t *result=(uint8_t *)malloc(2 * bufferSize);
    this->writeTo(result,0,0,
        text,bmWidth,this->height,
        foreColour,backColour);
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

//This algorithm depends on glyph width being a multiple of 4
//buffer is the destination buffer.
//x, y are the screen coords to start printing.
//width, height are the screen dimensions of the buffer.
void Font::writeTo(uint8_t *buffer, size_t width, size_t height, string text, int x, int y, const Colour565 *foreColour, const Colour565 *backColour) {
    size_t start_mem_offset=y * width + x;
    size_t lines_to_draw=min((int)this->height,(int)height-y);
    for (size_t k=0;k<text.length();++k) {
        //for each letter. Grab its index from the vector:
        uint8_t next_c=text[k];
        auto search = this->bitmaps.find(next_c);
        if (search != this->bitmaps.end()) {
            //if it has a bitmap:
            uint8_t *char_bits = search->second;
            //Keep the current byte of data in scope across row/column loop.
            uint8_t current_byte;
            for (uint8_t j=0;j<lines_to_draw;++j) {
                //for each row...
                for (uint8_t i=0;i<this->width;i+=4) {
                    //for each column. We're stepping by half 
                    current_byte = char_bits[(j * this->width + i)/8];
//                    ESP_LOGI(TAG,"draw %i",current_byte);
                    int base_offset=start_mem_offset + i*2;
                    const Colour565 *colour = (current_byte & 0x08)?foreColour:backColour;
                    buffer[base_offset] = colour->high;
                    buffer[base_offset + 1] = colour->low;
                    colour = (current_byte & 0x04)?foreColour:backColour;
                    buffer[base_offset + 2] = colour->high;
                    buffer[base_offset + 3] = colour->low;
                    colour = (current_byte & 0x02)?foreColour:backColour;
                    buffer[base_offset + 4] = colour->high;
                    buffer[base_offset + 5] = colour->low;
                    colour = (current_byte & 0x01)?foreColour:backColour;
                    buffer[base_offset + 6] = colour->high;
                    buffer[base_offset + 7] = colour->low;
                }
                start_mem_offset+=width;
            }
        }
        start_mem_offset+=this->width;
    }
}

