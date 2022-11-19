#include <parser/parser.hxx>
#include <lexer/lexer.hxx>
#include <preprocessor/preprocessor.hxx>

Parser::Parser(const std::string& input)
    : input_(input)
{
}

Parser::~Parser() {}

void Parser::Parse() {
    Preprocessor preprocessor(input_);
}