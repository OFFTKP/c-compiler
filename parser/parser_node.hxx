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

struct ModifyExpressionNode : public ASTNode {
    ModifyExpressionNode() : ASTNode(ASTNodeType::ModifyExpression, {}) {}
    ASTNodePtr LHS, RHS;
    TokenType LHSType, RHSType;
    TokenType Op;

    static ASTNodePtr Create(ASTNodePtr lhs, ASTNodePtr rhs, TokenType lhstype, TokenType rhstype, TokenType op) {
        auto node = std::make_unique<ModifyExpressionNode>();
        node->LHS = std::move(lhs);
        node->RHS = std::move(rhs);
        node->LHSType = lhstype;
        node->RHSType = rhstype;
        node->Op = op;
        return node;
    }
};

struct BlockItemListNode : public ASTNode {
    BlockItemListNode() : ASTNode(ASTNodeType::BlockItemList, {}) {}
    static ASTNodePtr Create(ASTNodePtr first) {
        // Squash the list into a single node
        auto node = std::make_unique<BlockItemListNode>();
        auto cur_node = first.get();
        while (true) {
            node->Next.push_back(std::move(cur_node->Next[0]));
            if (cur_node->Next.size() == 1) {
                break;
            }
            cur_node = cur_node->Next[1].get();
        }
        return node;
    }
};

struct StructDeclaratorListNode : public ASTNode {
    StructDeclaratorListNode() : ASTNode(ASTNodeType::StructDeclaratorList, {}) {}
    static ASTNodePtr Create(ASTNodePtr first) {
        // Squash the list into a single node
        auto node = std::make_unique<StructDeclaratorListNode>();
        auto cur_node = first.get();
        while (true) {
            node->Next.push_back(std::move(cur_node->Next[0]));
            if (cur_node->Next.size() == 1) {
                break;
            }
            cur_node = cur_node->Next[1].get();
        }
        return node;
    }
};

struct StructDeclarationListNode : public ASTNode {
    StructDeclarationListNode() : ASTNode(ASTNodeType::StructDeclarationList, {}) {}
    static ASTNodePtr Create(ASTNodePtr first) {
        // Squash the list into a single node
        auto node = std::make_unique<StructDeclarationListNode>();
        auto cur_node = first.get();
        while (true) {
            node->Next.push_back(std::move(cur_node->Next[0]));
            if (cur_node->Next.size() == 1) {
                break;
            }
            cur_node = cur_node->Next[1].get();
        }
        return node;
    }
};

struct EqualityExpressionNode : public ASTNode {
    EqualityExpressionNode() : ASTNode(ASTNodeType::EqualityExpression, {}) {}
    static ASTNodePtr Create(ASTNodePtr first) {
        // Squash the list into a single node
        auto node = std::make_unique<EqualityExpressionNode>();
        auto cur_node = first.get();
        while (true) {
            node->Next.push_back(std::move(cur_node->Next[0]));
            if (cur_node->Next.size() == 1) {
                break;
            }
            cur_node = cur_node->Next[1].get();
        }
        return node;
    }
};

struct AndExpressionNode : public ASTNode {
    AndExpressionNode() : ASTNode(ASTNodeType::AndExpression, {}) {}
    static ASTNodePtr Create(ASTNodePtr first) {
        // Squash the list into a single node
        auto node = std::make_unique<AndExpressionNode>();
        auto cur_node = first.get();
        while (true) {
            node->Next.push_back(std::move(cur_node->Next[0]));
            if (cur_node->Next.size() == 1) {
                break;
            }
            cur_node = cur_node->Next[1].get();
        }
        return node;
    }
};

struct XorExpressionNode : public ASTNode {
    XorExpressionNode() : ASTNode(ASTNodeType::ExclusiveOrExpression, {}) {}
    static ASTNodePtr Create(ASTNodePtr first) {
        // Squash the list into a single node
        auto node = std::make_unique<XorExpressionNode>();
        auto cur_node = first.get();
        while (true) {
            node->Next.push_back(std::move(cur_node->Next[0]));
            if (cur_node->Next.size() == 1) {
                break;
            }
            cur_node = cur_node->Next[1].get();
        }
        return node;
    }
};

struct OrExpressionNode : public ASTNode {
    OrExpressionNode() : ASTNode(ASTNodeType::InclusiveOrExpression, {}) {}
    static ASTNodePtr Create(ASTNodePtr first) {
        // Squash the list into a single node
        auto node = std::make_unique<OrExpressionNode>();
        auto cur_node = first.get();
        while (true) {
            node->Next.push_back(std::move(cur_node->Next[0]));
            if (cur_node->Next.size() == 1) {
                break;
            }
            cur_node = cur_node->Next[1].get();
        }
        return node;
    }
};

struct LogicalOrExpressionNode : public ASTNode {
    LogicalOrExpressionNode() : ASTNode(ASTNodeType::LogicalOrExpression, {}) {}
    static ASTNodePtr Create(ASTNodePtr first) {
        // Squash the list into a single node
        auto node = std::make_unique<LogicalOrExpressionNode>();
        auto cur_node = first.get();
        while (true) {
            node->Next.push_back(std::move(cur_node->Next[0]));
            if (cur_node->Next.size() == 1) {
                break;
            }
            cur_node = cur_node->Next[1].get();
        }
        return node;
    }
};

struct LogicalAndExpressionNode : public ASTNode {
    LogicalAndExpressionNode() : ASTNode(ASTNodeType::LogicalAndExpression, {}) {}
    static ASTNodePtr Create(ASTNodePtr first) {
        // Squash the list into a single node
        auto node = std::make_unique<LogicalAndExpressionNode>();
        auto cur_node = first.get();
        while (true) {
            node->Next.push_back(std::move(cur_node->Next[0]));
            if (cur_node->Next.size() == 1) {
                break;
            }
            cur_node = cur_node->Next[1].get();
        }
        return node;
    }
};

struct RelationalExpressionNode : public ASTNode {
    RelationalExpressionNode() : ASTNode(ASTNodeType::RelationalExpression, {}) {}
    static ASTNodePtr Create(ASTNodePtr first) {
        // Squash the list into a single node
        auto node = std::make_unique<RelationalExpressionNode>();
        auto cur_node = first.get();
        while (true) {
            node->Next.push_back(std::move(cur_node->Next[0]));
            if (cur_node->Next.size() == 1) {
                break;
            }
            cur_node = cur_node->Next[1].get();
        }
        return node;
    }
};

struct ShiftExpressionNode : public ASTNode {
    ShiftExpressionNode() : ASTNode(ASTNodeType::ShiftExpression, {}) {}
    static ASTNodePtr Create(ASTNodePtr first) {
        // Squash the list into a single node
        auto node = std::make_unique<ShiftExpressionNode>();
        auto cur_node = first.get();
        while (true) {
            node->Next.push_back(std::move(cur_node->Next[0]));
            if (cur_node->Next.size() == 1) {
                break;
            }
            cur_node = cur_node->Next[1].get();
        }
        return node;
    }
};

struct AdditiveExpressionNode : public ASTNode {
    AdditiveExpressionNode() : ASTNode(ASTNodeType::AdditiveExpression, {}) {}
    static ASTNodePtr Create(ASTNodePtr first) {
        // Squash the list into a single node
        auto node = std::make_unique<AdditiveExpressionNode>();
        auto cur_node = first.get();
        while (true) {
            node->Next.push_back(std::move(cur_node->Next[0]));
            if (cur_node->Next.size() == 1) {
                break;
            }
            cur_node = cur_node->Next[1].get();
        }
        return node;
    }
};

struct MultiplicativeExpressionNode : public ASTNode {
    MultiplicativeExpressionNode() : ASTNode(ASTNodeType::MultiplicativeExpression, {}) {}
    static ASTNodePtr Create(ASTNodePtr first) {
        // Squash the list into a single node
        auto node = std::make_unique<MultiplicativeExpressionNode>();
        auto cur_node = first.get();
        while (true) {
            node->Next.push_back(std::move(cur_node->Next[0]));
            if (cur_node->Next.size() == 1) {
                break;
            }
            cur_node = cur_node->Next[1].get();
        }
        return node;
    }
};

inline constexpr auto MakeNode = std::make_unique<ASTNode, ASTNodeType, std::vector<ASTNodePtr>>;
#endif