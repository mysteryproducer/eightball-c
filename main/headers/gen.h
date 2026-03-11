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
        protected:
            const string whitespace = " \t\n\r\f\v";
            string trim(const string& str) {
                size_t start = str.find_first_not_of(whitespace);
                if (start == std::string::npos) {
                    return ""; // String is all whitespace
                }
                size_t end = str.find_last_not_of(whitespace);
                return str.substr(start, end - start + 1);
            }
    };
};

//Factory function to create a TextGenerator based on the provided configuration
EightBall::TextGenerator *initGenerator(gen_config config);

namespace EightBall {
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