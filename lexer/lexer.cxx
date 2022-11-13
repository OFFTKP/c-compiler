#include <lexer/lexer.hxx>
#include <regex>
#include <iostream>
#include <iomanip>
#include <common/error.hxx>
#include <misc/scope_guard.hxx>

Lexer::Lexer(const std::string& input)
    : input_(input)
    , index_(input.begin())
    , next_token_(Token::Empty)
    , is_string_literal_(false)
{}

Lexer::~Lexer() {}

std::tuple<Token, std::string> Lexer::GetNextToken()
{
    ScopeGuard guard([&]() {
        next_token_string_ = "";
    });
    while (index_ != input_.end()) {
        if (!check(*index_)) {
            ++index_;
            return { get_type(), next_token_string_ };
        } else {
            ++index_;
        }
    }
    if (is_string_literal_)
        throw std::runtime_error("Unfinished string literal!");
    return { Token::Eof, "" };
}

char Lexer::peek(int i) {
    return *std::next(index_, i);
}

char Lexer::prev() {
    return *std::prev(index_);
}

// Check if character belongs to current token
bool Lexer::check(char c)
{
    if (is_string_literal_) {
        if (c == '"' && prev() != '\\')
            return false;
        // Add any character other than " to the string
        next_token_string_ += c;
        return true;
    }

    if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
        return next_token_string_.empty();

    if (std::regex_match(std::string(1, c), std::regex("[A-Za-z0-9_]"))) {
        next_token_string_ += c;
        return true;
    }

    if (!next_token_string_.empty()) {
        --index_;
        return false;
    }

    switch (c) {
        case '>':
        case '<': {
            next_token_string_ += c;
            char n = peek(1);
            if (n == c) {
                ++index_;
                next_token_string_ += c;
                char n2 = peek(1);
                if (n2 == '=') {
                    ++index_;
                    next_token_string_ += *index_;
                }
            } else if (n == '=') {
                ++index_;
                next_token_string_ += *index_;
            }
            return false;
        }
        case '=':
        case '%':
        case '*':
        case '^':
        case '/':
        case '!': {
            next_token_string_ += c;
            char n = peek(1);
            if (n == '=') {
                ++index_;
                next_token_string_ += *index_;
            }
            return false;
        }
        case '-':
        case '+': {
            next_token_string_ += c;
            char n = peek(1);
            if (n == '=' || n == c || (n == '>' && c == '-')) {
                ++index_;
                next_token_string_ += *index_;
            }
            return false;
        }
        case '|':
        case '&': {
            next_token_string_ += c;
            char n = peek(1);
            if (n == '=' || n == c) {
                ++index_;
                next_token_string_ += *index_;
            }
            return false;
        }
        case '~':
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
            next_token_string_ += c;
            return false;
        }
        case '"': {
            is_string_literal_ = true;
            return true;
        }
        default: {
            ERROR("Unknown character: 0x" << std::setfill('0') << std::setw(2) << std::hex << (int)c);
        }
    }

    return true;
}

Token Lexer::get_type()
{
    #define match(str) std::regex_match(next_token_string_, std::regex(str))
    #define matchw(str) next_token_string_ == str
    if (is_string_literal_) {
        is_string_literal_ = false;
        return Token::StringLiteral;
    } else if (matchw("char")) {
        return Token::Char;
    } else if (matchw("const")) {
        return Token::Const;
    } else if (matchw("if")) {
        return Token::If;
    } else if (matchw("int")) {
        return Token::Int;
    } else if (matchw("return")) {
        return Token::Return;
    } else if (matchw(">>=")) {
        return Token::RightAssign;
    } else if (matchw("<<=")) {
        return Token::LeftAssign;
    } else if (matchw("+=")) {
        return Token::AddAssign;
    } else if (matchw("-=")) {
        return Token::SubAssign;
    } else if (matchw("*=")) {
        return Token::MulAssign;
    } else if (matchw("/=")) {
        return Token::DivAssign;
    } else if (matchw("%=")) {
        return Token::ModAssign;
    } else if (matchw("&=")) {
        return Token::AndAssign;
    } else if (matchw("^=")) {
        return Token::XorAssign;
    } else if (matchw("|=")) {
        return Token::OrAssign;
    } else if (matchw("++")) {
        return Token::IncOp;
    } else if (matchw("--")) {
        return Token::DecOp;
    } else if (matchw("==")) {
        return Token::EqOp;
    } else if (matchw("!=")) {
        return Token::NeOp;
    } else if (matchw("||")) {
        return Token::OrOp;
    } else if (matchw("&&")) {
        return Token::AndOp;
    } else if (matchw(">=")) {
        return Token::GeOp;
    } else if (matchw("<=")) {
        return Token::LeOp;
    } else if (match("[;\\{\\},:=\\(\\)\\[\\].&!~\\-\\+\\*/%<>^\\|?]")) {
        return Token::Punctuator; 
    } else if (match("[A-Za-z_][A-Za-z0-9_]*")) {
        return Token::Identifier;
    } else if (match("[0-9]+")) {
        return Token::IntegerConstant;
    }
    #undef match
    ERROR("Unknown token:" << next_token_string_);
}
