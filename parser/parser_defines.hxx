#ifndef PARSER_DEFINES_HXX
#define PARSER_DEFINES_HXX
#define MkNd(type) MakeNode(ASTNodeType::type, std::move(next), "")
#define MATCH_ANY(...) check_many(get_token_type(), {__VA_ARGS__})
#define RETURN_IF_MATCH(type, ...)  \
    GRAMMAR; \
    bool ret = MATCH_ANY(__VA_ARGS__); \
    auto node = ret ? MkNd(type) : nullptr; \
    if (node) \
        node->Value = deserialize(get_token_type()); \
    advance_if(ret); \
    return node;
// #define GET_UNUSED_NAME(name) (std::to_string(++value_count_[name]) + std::string("__") + std::string(name))
#define GRAMMAR std::vector<ASTNodePtr> next
#define MAKE_SIMPLE_LIST(list_func, member_func, type, expression) \
    ASTNodePtr is_##list_func() { \
        GRAMMAR; \
        if (auto mem_node = is_##member_func()) { \
            auto _list_node = _is_##list_func(); \
            next.push_back(std::move(mem_node)); \
            if (_list_node) next.push_back(std::move(_list_node)); \
            return MkNd(type); \
        } \
        return nullptr; \
    } \
    ASTNodePtr _is_##list_func() { \
        GRAMMAR; \
        if (expression) { \
            if (auto mem_node = is_##member_func()) { \
                next.push_back(std::move(mem_node)); \
                if (auto list_node = _is_##list_func()) { \
                    next.push_back(std::move(list_node)); \
                } \
                return MkNd(type); \
            } \
        } \
        return nullptr; \
    }


#endif