#ifndef PARSER_NODE_HXX
#define PARSER_NODE_HXX
#include <memory>
#include <vector>

enum class ASTNodeType {
    #define DEF(type) type,
    #include <parser/parser_nodes.def>
    #undef DEF
};
static inline constexpr std::string deserialize(ASTNodeType e) {
    std::string result, temp;
    switch (e) {
        #define DEF(x) case ASTNodeType::x: temp = #x; break;
        #include <parser/parser_nodes.def>
        #undef DEF
    }
    for (int i = 0; i < temp.size(); ++i) {
        auto c = temp[i];
        if (c >= 'A' && c <= 'Z') {
            if (i != 0) {
                result += "_";
            }
            result += tolower(c);
        } else {
            result += c;
        }
    }
    return result;
}

struct ASTNode;
using ASTNodePtr = std::unique_ptr<ASTNode>;
struct ASTNode {
    ASTNode(ASTNodeType type, std::vector<ASTNodePtr> next, std::string value = "") : Type(type), Next(std::move(next)) {}
    ASTNodeType Type;
    std::vector<ASTNodePtr> Next;
    std::string GetUniqueName() { return unique_name_; };
    void SetUniqueName(std::string name) { unique_name_ = name; };
private:
    std::string unique_name_;
};
inline constexpr auto MakeNode = std::make_unique<ASTNode, ASTNodeType, std::vector<ASTNodePtr>, std::string>;
#endif