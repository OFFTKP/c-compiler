#ifndef LEXER_HXX
#define LEXER_HXX
#include <string>
#include <tuple>
#include <token/token.hxx>
#include <common/uncopyable.hxx>

class Lexer : public Uncopyable {
public:
    Lexer(const std::string& input);
    ~Lexer();

    std::vector<Token> Lex();
    Token GetNextTokenType();
    void Restart();
private:
    const std::string& input_;
    std::string::const_iterator index_;
    TokenType next_token_;
    std::string next_token_string_;
    bool is_string_literal_;
    char peek(int i);
    char prev();
    bool check(char c);
    TokenType get_type();
};
#endif