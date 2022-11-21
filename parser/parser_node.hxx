#ifndef PARSER_NODE_HXX
#define PARSER_NODE_HXX
#include <common/uncopyable.hxx>
#include <vector>

enum class ParserNodeType {
    #define DEF(type) type,
    #include <parser/parser_nodes.def>
    #undef DEF
};

struct ParserNode{
    ParserNodeType type = ParserNodeType::Start;
    std::vector<std::shared_ptr<ParserNode>> next_nodes = {};
};
#endif