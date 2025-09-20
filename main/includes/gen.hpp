#include <string>
#include <vector>
#include <map>
using namespace std;

class TextGenerator {
    public:
        virtual string generateNext();
        virtual ~TextGenerator() {}
};

static TextGenerator* generator;

class GrammarGenerator : public TextGenerator {
    public:
        GrammarGenerator(char* filename);
        string generateNext();
        //~GrammarGenerator();
    private:
        void readFile(char* filename);
        map<string,vector<string>> substitutions;
        vector<string> templates;
        string getRandomElement(vector<string>* items);
        vector<string> *getOptions(string key); 
};