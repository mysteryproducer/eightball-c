#pragma once
#include <string>
#include <vector>
#include <map>

using namespace std;

namespace EightBall {

class TextGenerator {
    public:
        virtual string generateNext() {return "";};
        virtual ~TextGenerator() {};
};

class GrammarGenerator : public TextGenerator {
    public:
        GrammarGenerator(const char* filename);
        string generateNext() override;
        ~GrammarGenerator() override {};
    private:
        void readFile(const char* filename);
        map<string,vector<string> *> substitutions;
        vector<string> templates;

        string getRandomElement(vector<string>* items);
        vector<string> *getOptions(string key); 
};

}