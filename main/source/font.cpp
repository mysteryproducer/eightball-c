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
    if (init_filesystem() != ESP_OK) {
        ESP_LOGE(TAG,"Couldn't init filesystem. Failed to load %s",path);
    }
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
            uint8_t ord = sz_byte[0];
            infile.read(letterMap, this->bytesPerChar);
            if (infile.gcount() < this->bytesPerChar) {
                ESP_LOGE(TAG,"Bad file alignment. Failed to load letter %i from %s",ord,path);
            }
            uint8_t *lmap=reinterpret_cast<uint8_t *>(letterMap);
            this->bitmaps.push_back(LetterBitmap { .ordinal=ord, .bitmap=lmap });
        }
    } else {
        ESP_LOGE(TAG,"Couldn't init filesystem. Failed to load %s",path);
    }
    delete[] sz_byte;
    close_filesystem();
    return result;
}
uint8_t *Font::write(string text,short foreColour,short backColour) {
    int bmWidth=this->width*text.length();
    int bufferSize=bmWidth * this->height;
    uint8_t *result=(uint8_t *)malloc(2 * bufferSize);
    this->writeTo(result,0,0,bmWidth,this->height,text,foreColour,backColour);
    return result;
}

uint8_t binarySearch(vector<LetterBitmap> *letters, char letter) {
    uint8_t min = 0;
    uint8_t max = letters->size()-1;
    while(min<=max) {
        int mid=(min+max)/2;
        if ((*letters)[mid].ordinal == letter) {
            return mid;
        } else if ((*letters)[mid].ordinal > letter) {
            min = mid+1;
        } else {
            max = mid-1; 
        }
    }
    return -1;
}

//This algorithm depends on glyph width being a multiple of 4
//buffer is the destination buffer.
//x, y are the screen coords to start printing.
//width, height are the screen dimensions of the buffer.
void Font::writeTo(uint8_t *buffer, int x, int y, int width, int height, string text, short foreColour, short backColour) {
    uint8_t *fore=new uint8_t[2];
    fore[0]=(uint8_t)(foreColour&0xFF);
    fore[1]=(uint8_t)((foreColour>>8)&0xFF);
    uint8_t *back=new uint8_t[2];
    back[0]=(uint8_t)(backColour&0xFF);
    back[1]=(uint8_t)((backColour>>8)&0xFF);
    int start_mem_offset=y * width + x;
    int lines_to_draw=min((int)this->height,height-y);
    for (int k=0;k<text.length();++k) {
        //for each letter. Grab its index from the vector:
        uint8_t next_c=text[k];
        uint8_t index=binarySearch(&this->bitmaps,next_c);
        if (index >= 0) {
            //if it has a bitmap:
            uint8_t *char_bits=this->bitmaps[index].bitmap;
            //Keep the current byte of data in scope across row/column loop.
            uint8_t current_byte;
            for (uint8_t j=0;j<lines_to_draw;++j) {
                //for each row...
                for (uint8_t i=0;i<this->width;i+=4) {
                    //for each column. We're stepping by half 
                    current_byte = char_bits[(j * this->width + i)/8];
                    int base_offset=start_mem_offset + i*2;
                    uint8_t *colour = (current_byte & 0x08)?fore:back;
                    buffer[base_offset] = colour[1];
                    buffer[base_offset + 1] = colour[0];
                    colour = (current_byte & 0x04)?fore:back;
                    buffer[base_offset + 2] = colour[1];
                    buffer[base_offset + 3] = colour[0];
                    colour = (current_byte & 0x02)?fore:back;
                    buffer[base_offset + 4] = colour[1];
                    buffer[base_offset + 5] = colour[0];
                    colour = (current_byte & 0x01)?fore:back;
                    buffer[base_offset + 6] = colour[1];
                    buffer[base_offset + 7] = colour[0];
                }
                start_mem_offset+=width;
            }
        }
        start_mem_offset+=this->width;
    }
}

