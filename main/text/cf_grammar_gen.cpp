#include <stdio.h>
#include <cstdlib>
#include <ctime>
#include "esp_log.h"
#include "esp_system.h"
//#include "esp_spiffs.h"

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
    string outer=getRandomElement(this->templates);
//    string outer="{nd.ind|nil}{nd.sev}{nd.loc|nil}Neurocognitive Disorder{nd.due|nil}"s;
    int subIndex=outer.find('{');
    while (subIndex != string::npos) {
        int endIndex=outer.find('}',subIndex+1);
        if (endIndex == string::npos) {
            break;
        }
        string key=outer.substr(subIndex+1,endIndex-subIndex-1);
        ESP_LOGD(TAG,"sub lookup [%s]: %D - %i [%s]",outer.c_str(),subIndex,endIndex,key.c_str());
        
        string insert=this->getSubstitute(key);
        if (insert.length() > 0) {
            if (outer.length() > endIndex+1) {
                if (outer.at(endIndex+1) != ' ') {
                    insert=insert+' ';
                }
            } else if (outer.at(subIndex-1) != ' ') {
                insert=' ' + insert;
            }
        } 
//        ESP_LOGD(TAG,"inserting '%s'",insert.c_str());
        outer.replace(subIndex,endIndex-subIndex+1,insert);
//        ESP_LOGD(TAG,"%s",outer.c_str());
        subIndex=outer.find('{');
    }
    return outer;
}

string GrammarGenerator::getSubstitute(const string &key) {
    ESP_LOGD(TAG,"lookup key: '%s'",key.c_str());
    vector<string> keys;
    int lastPos=0;
    int index=key.find("|"s);
    while(index!=key.npos) {
        string subkey=key.substr(lastPos,index-lastPos);
//        ESP_LOGD(TAG,"adding option set '%s'",subkey.c_str());
        keys.push_back(subkey);
        lastPos=index+1;
        index=key.find("|"s,lastPos);
    }
    string lastKey=key.substr(lastPos,key.length()-lastPos);
    keys.push_back(lastKey);
//    ESP_LOGD(TAG,"adding option set '%s'",lastKey.c_str());
    string finalKey=this->getRandomElement(keys);
    vector<string> result = this->substitutions[finalKey];
//    ESP_LOGD(TAG,"selected '%s'; %i options",finalKey.c_str(),result.size());
    return getRandomElement(result);
}

string GrammarGenerator::getRandomElement(const vector<string> &items) {
    int index=rand() % items.size();
//    ESP_LOGI(TAG,"Picked item %i",index);
    return items.at(index);
}

const string whitespace = " \t\n\r\f\v";
string trim(const string& str) {
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return ""; // String is all whitespace
    }
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

void GrammarGenerator::readFile(const char* filename) {
    ESP_LOGD(TAG,"Reading file %s",filename);
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
            string line=trim(linePtr);
            if (line.front()=='#') {
                continue;
            }
            if (line.empty()) {
                continue;
            }
            found++;
            if (line.at(0)=='[') {
                int endix=line.find("]");
                string key=line.substr(1,endix-1);
                auto iterator=this->substitutions.find(key);
                if (iterator==this->substitutions.end()) {
                    vector<string> sublist;
                    sublist.push_back(line.substr(endix+1));
                    this->substitutions[key] = sublist;
                } else {
                    iterator->second.push_back(line.substr(endix+1));
                }
            } else {
                this->templates.push_back(line);
            }
        }
        fclose(file);
    }
    close_filesystem();
    ESP_LOGD(TAG,"Finished reading file %s. %i lines processed.",filename,found);
}
