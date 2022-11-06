#ifndef PREPROCESSOR_HXX
#define PREPROCESSOR_HXX
#include <string>

struct Preprocessor {
    static std::string Process(const std::string& input);
private:
    static std::string remove_comments(const std::string& input);
};
#endif