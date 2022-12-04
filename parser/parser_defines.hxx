#ifndef PARSER_DEFINES_HXX
#define PARSER_DEFINES_HXX
#define MkNd(type) MakeNode(ASTNodeType::type, std::move(next), "")
#define MATCH_ANY(...) check_many(get_token_type(), {__VA_ARGS__})
#define parser_error {std::stringstream ss; ss << "Parser error (" << __func__ << "):\nAt index:" << index_ - tokens_.begin(); throw std::runtime_error(ss.str());}
#define RETURN_IF_MATCH(type, ...)  \
    std::vector<ASTNodePtr> next; \
    bool ret = MATCH_ANY(__VA_ARGS__); \
    auto node = ret ? MkNd(type) : nullptr; \
    if (node) \
        node->Value = deserialize(get_token_type()); \
    advance_if(ret); \
    return node;
// #define GET_UNUSED_NAME(name) (std::to_string(++value_count_[name]) + std::string("__") + std::string(name))
#endif