#ifndef PARSER_DEFINES_HXX
#define PARSER_DEFINES_HXX
#define MATCH_ANY(...) check_many(get_token_type(), {__VA_ARGS__})
#define RETURN_IF_MATCH(type, ...) bool ret = MATCH_ANY(__VA_ARGS__); \
    auto node = ret ? MakeNode(ASTNodeType::type, {}, GET_UNUSED_NAME(get_token_value())) : nullptr; \
    advance_if(ret); \
    return node;
#define GET_UNUSED_NAME(name) (std::to_string(++value_count_[name]) + std::string("__") + std::string(name))

#define MAKE_LIST(list_func, member_func, type) ASTNodePtr list_1 = MakeNode(ASTNodeType::type, {}, GET_UNUSED_NAME(#list_func)); \
    if (auto member_node = is_##member_func()) { \
        if (auto list_2 = is_##list_func()) \
            for (auto& node : list_2->Next) \
                list_1->Next.push_back(std::move(node)); \
        list_1->Next.push_back(std::move(member_node)); \
        return list_1; \
    } \
    return nullptr
#endif