#pragma once
#include <string>
#include <vector>
#include <map>

using namespace std;

namespace EightBall {

class TextGenerator {
    public:
        virtual string generateNext();
        virtual ~TextGenerator() {}
};

class GrammarGenerator : public TextGenerator {
    public:
        GrammarGenerator(const char* filename);
        string generateNext();
        //~GrammarGenerator();
    private:
        void readFile(const char* filename);
        map<string,vector<string>> substitutions;
        vector<string> templates;
        string getRandomElement(vector<string>* items);
        vector<string> *getOptions(string key); 
};

}