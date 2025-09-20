#include <stdio.h>
#include <cstdlib>
#include <ctime>

#include "esp_log.h"
#include "esp_spiffs.h"

#include "gen.hpp"
#include "gen.h"
#include "main.h"
#include "files.h"

GrammarGenerator::GrammarGenerator(char *file) {
    //seed prng
    srand(time(0));
    this->readFile(file);
}

string GrammarGenerator::generateNext() {
    string outer=getRandomElement(&this->templates);
    int subIndex=outer.find('{');
    while (subIndex>0) {
        int endIndex=outer.find('}',subIndex+1);
        if (endIndex<0) {
            break;
        }
        string key=outer.substr(subIndex+1,endIndex-subIndex-1);
        
        vector<string> *options=this->getOptions(key);
        string insert=this->getRandomElement(options);
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
    return &this->substitutions[finalKey];
}

string GrammarGenerator::getRandomElement(vector<string> *items) {
    int index=rand() % items->size();
    return items->at(index);
}

void GrammarGenerator::readFile(char* filename) {
    if (init_filesystem() != ESP_OK) {
        ESP_LOGE(TAG,"File system setup fail!",filename);
        return;
    }
    FILE *file = fopen(filename, "r");
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
            if (line->at(0)=='[') {
                int endix=line->find("]");
                string key=line->substr(1,endix-1);
                line->erase(0,endix+1);
                auto iterator=this->substitutions.find(key);
                vector<string> *sublist=NULL;
                if (iterator==this->substitutions.end()) {
                    sublist=new vector<string>();
                    this->substitutions[key]=*sublist;
                } else {
                    sublist=&iterator->second;
                }
                sublist->push_back(*line);
            } else {
                this->templates.push_back(*line);
            }
        }
        fclose(file);
    }
    esp_vfs_spiffs_unregister(NULL);
}

void test_gen() 
{
    FILE *file = fopen("/files/dsm5.txt", "r");
    if(file ==NULL)
    {
        ESP_LOGE(TAG,"File does not exist!");
    }
    else 
    {
        char line[256];
        while(fgets(line, sizeof(line), file) != NULL)
        {
            printf(line);
        }
        fclose(file);
    }
    esp_vfs_spiffs_unregister(NULL);
}