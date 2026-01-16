#include <string>
#include <vector>
#include "tft.h"
#include "math.h"
#include "esp_lcd_panel_ops.h"

using namespace std;
using namespace EightBall;

void deleteLines(vector <DisplayLine *> *layout) {
    for (DisplayLine *dl : *layout) {
        delete dl;
    }
    delete layout;
}

void splitString(string text,vector<string> *words) {
    size_t pos = text.find(' ');
    while (pos != string::npos) {
        string token = text.substr(0, pos);
        words->push_back(token);
        text.erase(0, pos + 1);
        pos = text.find(' ');
    }
    words->push_back(text);
    for (size_t i=0;i < words->size();++i) {
        string word=words->at(i);
        size_t h_ix=word.find('-');
        if (h_ix != string::npos) {
            string first = word.substr(0,h_ix); 
            words->insert(next(words->begin(),i),first);
            string second = word.substr(h_ix+1);
            words->at(i+1) = second;
        }
    }
}

void calculateBounds(vector<DisplayLine*> *lines, Font *font, Rectangle *result) {
    size_t minx=4096, maxx=0;
    size_t miny=4096, maxy=0;
    size_t fw=font->getWidth();
    size_t fh=font->getHeight();

    for (DisplayLine *line : *lines) {
        minx = min(minx, (size_t)max(0,line->xPos));
        miny = min(miny, (size_t)max(0,line->yPos));
        maxx = max(maxx, line->xPos + line->line.size() * fw);
        maxy = max(maxx, line->yPos + fh);
    }

    result->x = minx;
    result->width = maxx - minx;
    result->y = miny;
    result->height = maxy - miny;
}

esp_err_t EightBallScreen::drawText(string text) {
    vector<string> words;
//split the string
    splitString(text,&words);
    for (int i=0;i < fonts->size();++i) {
        Font *font = fonts->at(i);
        vector<DisplayLine *> *layout = layoutTextInCircle(&words,font);
        if (layout!=NULL) {
            for (int j=0;j<layout->size();++j) {
                font->writeText(this->screenBuffer,this,layout);
            }
            deleteLines(layout);
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FINISHED;
}
 

FontMetrics *EightBallScreen::getLinePositions(size_t font_height) {
    //look up screen metrics from previous runs:
    auto lookup = this->metrics.find(font_height);
    FontMetrics *fm;
    if (lookup==this->metrics.end()) {
        vector<size_t> *startPositions = new vector<size_t>();
        float diameter = (float)this->height - 10;
        size_t max_lines = (int)(diameter / font_height);
        size_t start_y = (this->height/2) - (max_lines/2) * font_height;
        for (size_t i=0;i<max_lines;++i) {
            startPositions->push_back(start_y + i*font_height);
        }
        fm = new FontMetrics {
            .screen_diameter = diameter,
            .radius = diameter/2.0f,
            .center_y = this->width/2,

            .max_lines = max_lines,
            .start_y = start_y,
            .positions = startPositions
        };
        this->metrics[font_height] = fm;
        return fm;
    } 
    return lookup->second;
}

// this function first drafted by GPT. Refactored and ported from python.
vector<DisplayLine *> *EightBallScreen::layoutTextInCircle(vector<string> *words,Font *font) {
    size_t font_width = font->getWidth();
    size_t font_height = font->getHeight();
    vector<DisplayLine *> *lines = new vector<DisplayLine *>();
    FontMetrics *metrics = this->getLinePositions(font_height);
    float radius = metrics->radius;

    string current_line = "";
    size_t word_index = 0;
    vector<size_t> *y_positions = metrics->positions;
    for (size_t y : *y_positions) {
        float dy = abs((float)y - radius);
        if (dy >= radius) {
            continue;  // Outside circle
        }

        float max_line_pixel_width = 2 * sqrt(radius*radius - dy*dy);
        int max_chars = int(max_line_pixel_width / font_width);

        string line = "";
        while (word_index < words->size()) {
            bool add_space = !(line.empty() || line.ends_with('-')); 
            string test_line = line + (add_space?" ":"") + words->at(word_index);
            if (test_line.length() <= max_chars) {
                line = test_line;
                word_index += 1;
            } else {
                break;
            }
        }
        if (!line.empty()) {
            size_t line_pixel_width = line.length() * font_width;
            size_t x = (metrics->screen_diameter - line_pixel_width) / 2;
            lines->push_back(new DisplayLine {
                .xPos = int(x)+5,
                .yPos = int(y)+5,
                .line = line 
            });
        }
        if (word_index >= words->size()) {
            break;
        }
    }
    if (word_index < words->size()) {
        deleteLines(lines);
        return NULL;
    }
    //eliminate gaps
    for (size_t i=1;i<lines->size();++i) {
        if (lines->at(i)->yPos - lines->at(i-1)->yPos > font_height) {
            for (int j=0;j<i;++j) {
                lines->at(j)->yPos += font_height;
            }
        }
    }
    //vertically centre
    int v_shift=int(metrics->radius - (lines->back()->yPos + font_height - lines->front()->yPos)/2);
    v_shift-=lines->front()->yPos;
    if (v_shift > 0) {
        for (DisplayLine *line : *lines) {
            line->yPos+=v_shift;
        }
    }
    return lines;
}