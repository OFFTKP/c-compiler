#include <lexer/lexer.hxx>
#include <regex>
#include <iostream>
#include <misc/scope_guard.hxx>

Lexer::Lexer(const std::string& input)
    : input_(input)
    , index_(input.begin())
    , next_token_(Token::Empty)
    , is_string_literal_(false)
{}

Lexer::~Lexer() {}

std::tuple<Token, std::string> Lexer::GetNextToken() {
    ScopeGuard guard([&]() {
        next_token_string_ = "";
    });
    while (index_ != input_.end()) {
        if (!check(*index_)) {
            ++index_;
            if (is_string_literal_)
                return { Token::StringLiteral, next_token_string_ };
            else
                return { get_type(), next_token_string_ };
        } else {
            ++index_;
        }
    }
    return { Token::Eof, "" };
}

char Lexer::peek(int i) {
    return *std::next(index_, i);
}

char Lexer::prev() {
    return *std::prev(index_);
}

// Check if character belongs to current token
bool Lexer::check(char c) {
    Token detected;

    if (is_string_literal_) {
        if (c == '"' && prev() != '\\') {
            is_string_literal_ = false;
            return false;
        }
        // Add any character other than " to the string
        return true;
    }

    if (c == ' ')
        return next_token_string_.empty();

    if (std::regex_match(std::string(1, c), std::regex("[A-Za-z0-9_]"))) {
        next_token_string_ += *index_;
        return true;
    }

    if (!next_token_string_.empty()) {
        --index_;
        return false;
    }

    switch (c) {
        case '>':
        case '<': {
            next_token_string_ += *index_;
            char n = peek(1);
            if (n == c) {
                ++index_;
                next_token_string_ += *index_;
                char n2 = peek(2);
                if (n2 == '=') {
                    ++index_;
                    next_token_string_ += *index_;
                }
            }
            return false;
        }
        case '=':
        case '%':
        case '*':
        case '/':
        case '!':
        case '~': {
            next_token_string_ += *index_;
            char n = peek(1);
            if (n == '=') {
                ++index_;
                next_token_string_ += *index_;
            }
            return false;
        }
        case '-':
        case '+': {
            next_token_string_ += *index_;
            char n = peek(1);
            if (n == '=' || n == c || (n == '>' && c == '-')) {
                ++index_;
                next_token_string_ += *index_;
            }
            return false;
        }
        case '|':
        case '&': {
            next_token_string_ += *index_;
            char n = peek(1);
            if (n == '=' || n == c) {
                ++index_;
                next_token_string_ += *index_;
            }
            return false;
        }
        case '^':
        case '?': 
        case ':':
        case ';':
        case '.':
        case ',':
        case '(':
        case ')':
        case '{': 
        case '}': 
        case '[':
        case ']': {
            next_token_string_ += *index_;
            return false;
        }
        case '"': {
            is_string_literal_ = true;
            return true;
        }
        default: {
            throw std::runtime_error(std::string("Unknown character:") + c);
        }
    }

    return true;
}

Token Lexer::get_type() {
    if (next_token_string_ == "int") {
        return Token::Int;
    } else if (std::regex_match(next_token_string_, std::regex("[A-Za-z_][A-Za-z0-9_]*"))) {
        return Token::IntegerConstant;
    } else if (std::regex_match(next_token_string_, std::regex("[0-9]+"))) {
        return Token::Identifier;
    }
    return Token::Empty;
    throw std::runtime_error("Unknown token:" + next_token_string_);
}
