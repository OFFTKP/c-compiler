#include <lexer/lexer.hxx>
#include <regex>
#include <iostream>
#include <iomanip>
#include <common/log.hxx>
#include <misc/scope_guard.hxx>

Lexer::Lexer(const std::string& input)
    : input_(input)
    , index_(input.begin())
    , next_token_(TokenType::Empty)
    , is_string_literal_(false)
{}

Lexer::~Lexer() {}

std::vector<Token> Lexer::Lex() {
    std::vector<Token> tokens;
    bool is_eof = false;
    while (!is_eof) {
        auto tok = GetNextTokenType();
        auto [type, _] = tok;
        is_eof = (type == TokenType::Eof);
        tokens.push_back(tok);
    }
    return tokens;
}

void Lexer::Restart() {
    is_string_literal_ = false;
    next_token_string_ = "";
    next_token_ = TokenType::Empty;
    index_ = input_.begin();
}

Token Lexer::GetNextTokenType()
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
        ERROR("Unfinished string literal")
    else if (!next_token_string_.empty()) {
        auto type = get_type();
        std::string ret;
        next_token_string_.swap(ret);
        return { type, ret };
    }
    return { TokenType::Eof, "" };
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
        next_token_string_ += c;
        if (c == '"' && prev() != '\\')
            return false;
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
        case '.': {
            char n = peek(1);
            char n2 = peek(2);
            if (n == '.' && n2 == '.') {
                index_ += 2;
                next_token_string_ = "...";
                return false;   
            }
            [[fallthrough]];
        }
        case '~':
        case '?': 
        case ':':
        case ';':
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
            next_token_string_ += c;
            return true;
        }
        default: {
            ERROR("Unknown character: 0x" << std::setfill('0') << std::setw(2) << std::hex << (int)c);
        }
    }

    return true;
}

TokenType Lexer::get_type()
{
    #define match(str) std::regex_match(next_token_string_, std::regex(str))
    #define matchw(str) next_token_string_ == str
    if (is_string_literal_) {
        is_string_literal_ = false;
        return TokenType::StringLiteral;
    } else if (matchw("...")) {
        return TokenType::Ellipsis;
    } else if (matchw(">>")) {
        return TokenType::RightOp;
    } else if (matchw("<<")) {
        return TokenType::LeftOp;
    } else if (matchw(">>=")) {
        return TokenType::RightAssign;
    } else if (matchw("<<=")) {
        return TokenType::LeftAssign;
    } else if (matchw("+=")) {
        return TokenType::AddAssign;
    } else if (matchw("-=")) {
        return TokenType::SubAssign;
    } else if (matchw("*=")) {
        return TokenType::MulAssign;
    } else if (matchw("/=")) {
        return TokenType::DivAssign;
    } else if (matchw("%=")) {
        return TokenType::ModAssign;
    } else if (matchw("&=")) {
        return TokenType::AndAssign;
    } else if (matchw("^=")) {
        return TokenType::XorAssign;
    } else if (matchw("|=")) {
        return TokenType::OrAssign;
    } else if (matchw("++")) {
        return TokenType::IncOp;
    } else if (matchw("--")) {
        return TokenType::DecOp;
    } else if (matchw("==")) {
        return TokenType::EqOp;
    } else if (matchw("!=")) {
        return TokenType::NeOp;
    } else if (matchw("||")) {
        return TokenType::OrOp;
    } else if (matchw("&&")) {
        return TokenType::AndOp;
    } else if (matchw(">=")) {
        return TokenType::GeOp;
    } else if (matchw("<=")) {
        return TokenType::LeOp;
    } else if (match("[;\\{\\},:=\\(\\)\\[\\].&!~\\-\\+\\*/%<>^\\|?]")) {
        return TokenType::Punctuator;
    } else if (auto tok = serialize_l(next_token_string_); tok != TokenType::Empty) {
        return tok;
    } else if (match("[A-Za-z_][A-Za-z0-9_]*")) {
        return TokenType::Identifier;
    } else if (match("[0-9]+")) {
        return TokenType::Constant;
    }
    #undef match
    ERROR("Unknown token:" << next_token_string_);
    return TokenType::Error;
}
