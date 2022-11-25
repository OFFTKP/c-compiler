#ifndef PARSER_DEFINES_HXX
#define PARSER_DEFINES_HXX
#define MATCH_ANY(...) check_many(get_token_type(), {__VA_ARGS__})
#define RETURN_IF_MATCH(type, name, ...) bool ret = MATCH_ANY(__VA_ARGS__); \
    auto node = ret ? MakeNode(ASTNodeType::type, {}, GET_UNUSED_NAME(name)) : nullptr; \
    advance_if(ret); \
    return node;
#define GET_UNUSED_NAME(name) (std::to_string(++value_count_[name]) + std::string("__") + std::string(name))              
#define MAKE_SIMPLE_LIST(list_func, member_func, type, expression) \
    ASTNodePtr is_##list_func() { \
        std::vector<ASTNodePtr> next; \
        if (auto mem_node = is_##member_func()) { \
            auto _list_node = _is_##list_func(); \
            next.push_back(std::move(mem_node)); \
            if (_list_node) next.push_back(std::move(_list_node)); \
            return MakeNode(ASTNodeType::type, std::move(next), GET_UNUSED_NAME(#list_func)); \
        } \
        return nullptr; \
    } \
    ASTNodePtr _is_##list_func() { \
        std::vector<ASTNodePtr> next; \
        if (expression) { \
            if (auto mem_node = is_##member_func()) { \
                next.push_back(std::move(mem_node)); \
                if (auto list_node = _is_##list_func()) { \
                    next.push_back(std::move(list_node)); \
                } \
                return MakeNode(ASTNodeType::type, std::move(next), GET_UNUSED_NAME(#list_func)); \
            } \
        } \
        return nullptr; \
    }


#endif