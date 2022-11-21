#ifndef PARSER_DEFINES_HXX
#define PARSER_DEFINES_HXX
#define MATCH_ANY(...) check_many(get_token_type(), {__VA_ARGS__})
#define RETURN_IF_MATCH(type, ...) bool ret = MATCH_ANY(__VA_ARGS__); \
    auto node = ret ? MakeNode(ASTNodeType::type, {}, get_token_value()) : nullptr; \
    advance_if(ret); \
    return node;
#define GET_UNUSED_NAME(name) (std::string(name) + std::string("__") + std::to_string(++value_count_[name]))
#endif