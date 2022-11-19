#ifndef PARSER_HXX
#define PARSER_HXX
#include <string>

class Parser {
public:
    Parser(const std::string& input);
    ~Parser();

    void Parse();
private:
    const std::string& input_;
};
#endif