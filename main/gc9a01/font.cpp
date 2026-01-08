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
// uint8_t *Font::write(string text,const Colour565 *foreColour,const Colour565 *backColour) {
//     size_t bmWidth=this->width*text.length();
//     size_t bufferSize=bmWidth * this->height;
//     uint8_t *result=(uint8_t *)malloc(2 * bufferSize);
//     for (int k=0;k<text.length();++k) {
//         for(int j=0;j<this->)
//     }
//     uint8_t next_c=text[k];

//     return result;
// }

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
            this->drawLetter(text[i],buffer,width,height,x-i*this->width,y,foreColour,backColour);
        }
    }
}

void Font::drawLetter(char character, uint8_t *buffer, const Colour565 *foreColour, const Colour565 *backColour) {
    size_t bufsize=this->width*this->height*2;
    size_t bytes_per_char = bufsize / 16;
    auto search = this->bitmaps.find(character);
    if (search != this->bitmaps.end()) {
        //if it has a bitmap:
        uint8_t *char_bits = search->second;
        for (size_t offset=0;offset<bytes_per_char;offset+=2) {
            size_t baseOffset = offset << 4;
            uint8_t in_byte=char_bits[offset];
            uint8_t in_byte2=char_bits[offset+1];
            buffer[baseOffset+30] =  (in_byte & 128)?foreColour->high:backColour->high;
            buffer[baseOffset+31] =  (in_byte & 128)?foreColour->low:backColour->low;
            buffer[baseOffset+28] =  (in_byte & 64)?foreColour->high:backColour->high;
            buffer[baseOffset+29] =  (in_byte & 64)?foreColour->low:backColour->low;
            buffer[baseOffset+26] =  (in_byte & 32)?foreColour->high:backColour->high;
            buffer[baseOffset+27] =  (in_byte & 32)?foreColour->low:backColour->low;
            buffer[baseOffset+24] =  (in_byte & 16)?foreColour->high:backColour->high;
            buffer[baseOffset+25] =  (in_byte & 16)?foreColour->low:backColour->low;
            buffer[baseOffset+22] =  (in_byte & 8)?foreColour->high:backColour->high;
            buffer[baseOffset+23] =  (in_byte & 8)?foreColour->low:backColour->low;
            buffer[baseOffset+20] =  (in_byte & 4)?foreColour->high:backColour->high;
            buffer[baseOffset+21] =  (in_byte & 4)?foreColour->low:backColour->low;
            buffer[baseOffset+18] =  (in_byte & 2)?foreColour->high:backColour->high;
            buffer[baseOffset+19] =  (in_byte & 2)?foreColour->low:backColour->low;
            buffer[baseOffset+16] =  (in_byte & 1)?foreColour->high:backColour->high;
            buffer[baseOffset+17] =  (in_byte & 1)?foreColour->low:backColour->low;
            buffer[baseOffset+14] =  (in_byte2 & 128)?foreColour->high:backColour->high;
            buffer[baseOffset+15] =  (in_byte2 & 128)?foreColour->low:backColour->low;
            buffer[baseOffset+12] =  (in_byte2 & 64)?foreColour->high:backColour->high;
            buffer[baseOffset+13] =  (in_byte2 & 64)?foreColour->low:backColour->low;
            buffer[baseOffset+10] =  (in_byte2 & 32)?foreColour->high:backColour->high;
            buffer[baseOffset+11] =  (in_byte2 & 32)?foreColour->low:backColour->low;
            buffer[baseOffset+8] =  (in_byte2 & 16)?foreColour->high:backColour->high;
            buffer[baseOffset+9] =  (in_byte2 & 16)?foreColour->low:backColour->low;
            buffer[baseOffset+6] =  (in_byte2 & 8)?foreColour->high:backColour->high;
            buffer[baseOffset+7] =  (in_byte2 & 8)?foreColour->low:backColour->low;
            buffer[baseOffset+4] =  (in_byte2 & 4)?foreColour->high:backColour->high;
            buffer[baseOffset+5] =  (in_byte2 & 4)?foreColour->low:backColour->low;
            buffer[baseOffset+2] =  (in_byte2 & 2)?foreColour->high:backColour->high;
            buffer[baseOffset+3] =  (in_byte2 & 2)?foreColour->low:backColour->low;
            buffer[baseOffset] =  (in_byte2 & 1)?foreColour->high:backColour->high;
            buffer[baseOffset+1] =  (in_byte2 & 1)?foreColour->low:backColour->low;
        }
    }
}
void Font::drawLetter(char character, uint8_t *buffer, size_t bufferWidth, size_t bufferHeight, size_t x, size_t y, const Colour565 *foreColour, const Colour565 *backColour) {
    auto search = this->bitmaps.find(character);
    //if it has a bitmap:
    if (search != this->bitmaps.end()) {
        size_t bytes_per_char = this->width * this->height / 8;
        //The x-axis needs flipping. Start at the end of the string and move backwards:
//        x += this->width;
        uint8_t *char_bits = search->second;
        for (size_t i=0;i<bytes_per_char;++i) {
            uint8_t in_byte=char_bits[i^1];
            size_t current_row = y + (i*8)/this->width;
            size_t current_column = x - this->width + (i*8) % this->width;
            size_t baseOffset = (current_row*bufferWidth + current_column)*2; //(byte * 8) / (width * 2)

            buffer[baseOffset-6] = (in_byte & 1)?foreColour->high:backColour->high;
            buffer[baseOffset-7] = (in_byte & 1)?foreColour->low:backColour->low;
            buffer[baseOffset-4] = (in_byte & 2)?foreColour->high:backColour->high;
            buffer[baseOffset-5] = (in_byte & 2)?foreColour->low:backColour->low;
            buffer[baseOffset-2] = (in_byte & 4)?foreColour->high:backColour->high;
            buffer[baseOffset-3] = (in_byte & 4)?foreColour->low:backColour->low;
            buffer[baseOffset] = (in_byte & 8)?foreColour->high:backColour->high;
            buffer[baseOffset-1] = (in_byte & 8)?foreColour->low:backColour->low;

            current_row = y + (i*8+4)/this->width;
            current_column = x - this->width + (i*8+4) % this->width;
            baseOffset = (current_row*bufferWidth + current_column)*2;
            buffer[baseOffset-6] = (in_byte & 16)?foreColour->high:backColour->high;
            buffer[baseOffset-7] = (in_byte & 16)?foreColour->low:backColour->low;
            buffer[baseOffset-4] = (in_byte & 32)?foreColour->high:backColour->high;
            buffer[baseOffset-5] = (in_byte & 32)?foreColour->low:backColour->low;
            buffer[baseOffset-2] = (in_byte & 64)?foreColour->high:backColour->high;
            buffer[baseOffset-3] = (in_byte & 64)?foreColour->low:backColour->low;
            buffer[baseOffset] = (in_byte & 128)?foreColour->high:backColour->high;
            buffer[baseOffset-1] = (in_byte & 128)?foreColour->low:backColour->low;

        }
    }
}

