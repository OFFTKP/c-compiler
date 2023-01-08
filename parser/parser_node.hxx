#ifndef PARSER_NODE_HXX
#define PARSER_NODE_HXX
#include <memory>
#include <vector>
#include <common/str_hash.hxx>
#include <token/token.hxx>

enum class ASTNodeType {
    #define DEF(type) type,
    #include <parser/parser_nodes.def> // for concrete nodes
    #include <parser/ast_nodes.def> // for abstract nodes
    #undef DEF
};

static inline constexpr std::string deserialize(ASTNodeType e) {
    switch (e) {
        #define DEF(x) case ASTNodeType::x: return #x;
        #include <parser/parser_nodes.def>
        #include <parser/ast_nodes.def>
        #undef DEF
    }
    throw std::runtime_error("Invalid ASTNodeType");
}

static inline constexpr ASTNodeType serialize_a(std::string e) {
    switch(hash(e.c_str())) {
        #define DEF(x) case hash(#x): return ASTNodeType::x;
        #include <parser/parser_nodes.def>
        #include <parser/ast_nodes.def>
        #undef DEF
    }
    throw std::runtime_error("Invalid ASTNodeType");
}

static inline constexpr std::string snake_case(std::string temp) {
    std::string result;
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
    ASTNode(ASTNodeType type, std::vector<ASTNodePtr> next) : Type(type), Next(std::move(next)) {}
    ASTNodeType Type;
    std::vector<ASTNodePtr> Next;
    std::string Value = "";
    std::string GetUniqueName() { return unique_name_; };
    void SetUniqueName(std::string name) { unique_name_ = name; };
    bool IsEmpty() { return Next.empty(); }
private:
    std::string unique_name_;
};

struct ModifyNode : public ASTNode {
    ModifyNode() : ASTNode(ASTNodeType::ModifyExpression, {}) {}
    ASTNodePtr LHS, RHS;
    TokenType LHSType, RHSType;
    TokenType Op;

    static ASTNodePtr Create(ASTNodePtr lhs, ASTNodePtr rhs, TokenType lhstype, TokenType rhstype, TokenType op) {
        auto node = std::make_unique<ModifyNode>();
        node->LHS = std::move(lhs);
        node->RHS = std::move(rhs);
        node->LHSType = lhstype;
        node->RHSType = rhstype;
        node->Op = op;
        return node;
    }
};

inline constexpr auto MakeNode = std::make_unique<ASTNode, ASTNodeType, std::vector<ASTNodePtr>>;
#endif