#ifndef PARSER_DEFINES_HXX
#define PARSER_DEFINES_HXX
#define MATCH_ANY(...) check_many(get_token_type(), {__VA_ARGS__})
#endif