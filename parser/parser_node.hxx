#ifndef PARSER_NODE_HXX
#define PARSER_NODE_HXX
#include <memory>
#include <vector>

enum class ASTNodeType {
    #define DEF(type) type,
    #include <parser/parser_nodes.def>
    #undef DEF
};
static inline std::string deserialize(ASTNodeType e) {
    switch (e) {
        #define DEF(x) case ASTNodeType::x: return #x;
        #include <parser/parser_nodes.def>
        #undef DEF
    }
    return "";
}
struct ASTNode;
using ASTNodePtr = std::unique_ptr<ASTNode>;
struct ASTNode {
    ASTNode(ASTNodeType type, std::vector<ASTNodePtr> next, std::string value = "") : Type(type), Next(std::move(next)), Value(value) {}
    ASTNodeType Type;
    std::string Value;
    std::vector<ASTNodePtr> Next;
};
inline constexpr auto MakeNode = std::make_unique<ASTNode, ASTNodeType, std::vector<ASTNodePtr>, std::string>;
#endif