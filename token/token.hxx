#ifndef TOKEN_HXX
#define TOKEN_HXX
enum class Token {
    #define DEF(x,y) x = y,
    #include <token/tokens.def>
    #undef DEF
};

static inline std::ostream & operator << (std::ostream &o, Token e) {
    switch ((int)e) {
        #define DEF(x, y) case (int)y: return o << std::string(#x);
        #include <token/tokens.def>
        #undef DEF
        default: return o;
    }
}
#endif