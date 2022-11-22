#ifndef TOKEN_HXX
#define TOKEN_HXX
#include <misc/json.hpp>
#include <common/str_hash.hxx>
#include <cctype>
#include <tuple>

enum class TokenType {
    Error,
    #define DEF(x,y) x = y,
    #include <token/tokens.def>
    #undef DEF
};

using Token = std::tuple<TokenType, std::string>;

static inline std::ostream& operator<<(std::ostream& o, TokenType e) {
    switch (e) {
        #define DEF(x, y) case TokenType::x: return o << std::string(#x);
        #include <token/tokens.def>
        #undef DEF
        default: return o;
    }
}

static inline std::string deserialize(TokenType e) {
    switch (e) {
        #define DEF(x, y) case TokenType::x: return #x;
        #include <token/tokens.def>
        #undef DEF
        default: return "";
    }
}

static inline TokenType serialize(std::string e) {
    switch (hash(e.c_str())) {
        #define DEF(x, y) case (hash(#x)): return TokenType::x;
        #include<token/tokens.def>
        #undef DEF
        default: return TokenType::Empty;
    }
}

static inline TokenType serialize_l(std::string e) {
    e[0] = std::toupper(e[0]);
    return serialize(e);
}

static inline std::ostream& operator<<(std::ostream& o, const std::vector<Token>& tokens) {
    nlohmann::json j;
    std::vector<nlohmann::json> objects;
    for (const auto& [type, value] : tokens) {
        nlohmann::json obj;
        obj[deserialize(type)] = value;
        objects.push_back(obj);
    }
    j["Tokens"] = objects;
    return o << j;
}

#endif