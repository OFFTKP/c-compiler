#ifndef STR_HASH_HXX
#define STR_HASH_HXX
constexpr unsigned int hash(const char *s, int off = 0) {                        
    return !s[off] ? 5381 : (hash(s, off+1)*33) ^ s[off];                           
}
#endif