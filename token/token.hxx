#ifndef TOKEN_HXX
#define TOKEN_HXX
enum class Token {
    Empty,
    Eof,
    Int,
    IntegerConstant,
    Identifier,
    StringLiteral,
};
#endif