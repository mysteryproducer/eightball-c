#pragma once
#include <string>
#include <vector>
#include <map>
#include "capi.h"

using namespace std;

namespace EightBall {

    #define GEN_TYPE_GRAMMAR "grammar"
    #define GEN_TYPE_FILE "file"
    #define GEN_TYPE_TEST "test"

    class TextGenerator {
        public:
            virtual string generateNext() {return "";};
            virtual ~TextGenerator() {};
    };

    class TestGenerator : public TextGenerator {
        public:
        TestGenerator() {};
        string generateNext() override {return "Substance/Medication-Induced Sexual Dysfunction";};
        ~TestGenerator() override {};
    };

    typedef struct {
        uint32_t from;
        uint32_t to;
    } line_bounds;

    class LineReader : public TextGenerator {
        public:
            LineReader(const char *file);
            string generateNext() override;
        private:
            const char *filename;
            void readOffsets();
            vector<line_bounds> lineOffsets;
    };

    class GrammarGenerator : public TextGenerator {
        public:
            GrammarGenerator(const char* filename);
            string generateNext() override;
            ~GrammarGenerator() override {};
        private:
            void readFile(const char* filename);
            map<string,vector<string>> substitutions;
            vector<string> templates;

            string getRandomElement(const vector<string> &items);
            string getSubstitute(const string &key); 
    };

}