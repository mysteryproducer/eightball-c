#include <stdio.h>
#include <cstdlib>
#include <ctime>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_spiffs.h"

#include "gen.h"
#include "files.h"

using namespace EightBall;
using namespace std;

static const char *TAG = "8 ball grammar generator";

GrammarGenerator::GrammarGenerator(const char *file) {
    this->readFile(file);
}

string GrammarGenerator::generateNext() {
    srand(time(0));
    string outer=getRandomElement(&this->templates);
    int subIndex=outer.find('{');
    while (subIndex != string::npos) {
        int endIndex=outer.find('}',subIndex+1);
        if (endIndex == string::npos) {
            break;
        }
        string key=outer.substr(subIndex+1,endIndex-subIndex-1);
        
        vector<string> *options=this->getOptions(key);
        string insert="";
        if (options!=NULL && options->size() > 0) { 
            insert=this->getRandomElement(options);
            if (outer[endIndex+1] != ' ') {
                insert+=' ';
            }
        }
        outer.replace(subIndex,endIndex-subIndex+1,insert);
    }
    return outer;
}

vector<string> *GrammarGenerator::getOptions(string key) {
    vector<string> keys;
    int lastPos=0;
    int index=key.find("|"s);
    while(index!=key.npos) {
        string subkey=key.substr(lastPos,index-lastPos);
        keys.push_back(subkey);
        lastPos=index+1;
        index=key.find("|"s,lastPos);
    }
    string lastKey=key.substr(lastPos,key.length()-lastPos);
    keys.push_back(lastKey);
    string finalKey=this->getRandomElement(&keys);
//    ESP_LOGI(TAG,"going with key %s",finalKey.c_str());
    vector<string> *result = this->substitutions[finalKey];
    return result;
}

string GrammarGenerator::getRandomElement(vector<string> *items) {
    int index=rand() % items->size();
    return items->at(index);
}

void GrammarGenerator::readFile(const char* filename) {
    ESP_LOGI(TAG,"Reading file %s",filename);
    if (init_filesystem() != ESP_OK) {
        ESP_LOGE(TAG,"File system setup fail!");
        return;
    }
    FILE *file = fopen(filename, "r");
    size_t found=0;
    if(file ==NULL)
    {
        ESP_LOGE(TAG,"File %s does not exist!",filename);
    }
    else 
    {
        char linePtr[1024];
        while(fgets(linePtr, sizeof(linePtr), file) != NULL) {
            if (linePtr[0]=='#') {
                continue;
            }
            string *line=new string(linePtr);
            if (line->empty()) {
                delete line;
                continue;
            }
            while(line->ends_with('\r') || line->ends_with('\n')) {
                line->pop_back();
            }
            found++;
            if (line->at(0)=='[') {
                int endix=line->find("]");
                string key=line->substr(1,endix-1);
                line->erase(0,endix+1);
                auto iterator=this->substitutions.find(key);
                vector<string> *sublist = NULL;
                if (iterator==this->substitutions.end()) {
                    sublist=new vector<string>();
                    this->substitutions[key] = sublist;
                } else {
                    sublist = iterator->second;
                }
                sublist->push_back(*line);
            } else {
                this->templates.push_back(*line);
            }
        }
        fclose(file);
    }
    close_filesystem();
    ESP_LOGI(TAG,"Finished reading file %s. %i lines processed.",filename,found);
}
