#include <lexer/lexer.hxx>
#include <parser/parser.hxx>
#include <preprocessor/preprocessor.hxx>
#include <common/log.hxx>
#include <regex>
#include <boost/stacktrace.hpp>

Parser::Parser(const std::string& input)
    : input_(input)
    , tokens_{}
    , index_(nullptr)
    , start_node_{}
{
    std::string unprocessed = input_;
    Preprocessor preprocessor(unprocessed);
    auto processed = preprocessor.Process();
    Lexer lexer(processed);
    tokens_ = lexer.Lex();
    index_ = tokens_.begin();
    assert(tokens_.size() > 0);
}

Parser::~Parser() {}

void Parser::Parse() {
    try {
        parse_impl();
    } catch (std::exception& ex) {
        find_error();
    }
    ++index_;
    assert(index_ == tokens_.end());
}

void Parser::find_error() {
    int i = index_ - tokens_.begin();
    int j = 0;
    int indentation = 0;
    bool first = false;
    std::cout << "Trying to find error:" << std::endl;
    for (auto& [tok, value] : tokens_) {
        if (j == i) {
            std::cout << "\033[31m";
        }
        if (first) {
            for (int k = 0; k < indentation; k++) {
                std::cout << "    ";
            }
            first = false;
        }
        std::cout << value;
        switch (hash(value.c_str())) {
            case hash("{"): {
                indentation++;
                std::cout << std::endl;
                first = true;
                break;
            }
            case hash(";"): {
                std::cout << std::endl;
                first = true;
                break;
            }
            default: {
                std::cout << " ";
                break;
            }
        }
        if (j == i) {
            std::cout << "\033[0m";
        }
        j++;
    }
}

void Parser::parser_error() {
    std::cout << boost::stacktrace::stacktrace() << std::endl;
    throw std::runtime_error("Parser error");
}

const ASTNodePtr& Parser::GetStartNode() {
    if (index_ != tokens_.end()) {
        ERROR("Parse failed before GetStartNode");
        parser_error();
    }
    return start_node_;
}

void Parser::parse_impl() {
    if (!(start_node_ = is_translation_unit()))
        parser_error();
}

ASTNodePtr Parser::is_translation_unit() {
    std::vector<ASTNodePtr> next;
    start_node_ = MkNd(Start);
    while(!MATCH_ANY(TokenType::Eof)) {
        if (auto ext = is_external_declaration()) {
            next.push_back(std::move(ext));
        }
    }
    start_node_->Next = std::move(next);
    return std::move(start_node_);
}

ASTNodePtr Parser::is_external_declaration() {
    if (auto decl_spec_node = is_declaration_specifiers()) {
        if (auto decl_node = is_declarator()) {
            auto decl_list_node = is_declaration_list();
            if (auto compound_node = is_compound_statement()) {
                std::vector<ASTNodePtr> temp;
                temp.push_back(std::move(decl_spec_node));
                temp.push_back(std::move(decl_node));
                if (decl_list_node) temp.push_back(std::move(decl_list_node));
                temp.push_back(std::move(compound_node));
                auto func_node = MakeNode(ASTNodeType::FunctionDefinition, std::move(temp));
                return func_node;
            }
        } else {
            auto init_decl_node = is_init_declarator_list();
            if (is_punctuator(';')) {
                std::vector<ASTNodePtr> temp;
                temp.push_back(std::move(decl_spec_node));
                if (init_decl_node) temp.push_back(std::move(init_decl_node));
                auto decl_node = MakeNode(ASTNodeType::Declaration, std::move(temp));
                return decl_node;
            } else {
                return nullptr;
            }
        }
    }
    parser_error();
    return nullptr;
}

ASTNodePtr Parser::is_function_definition() {
    std::vector<ASTNodePtr> next;
    if (auto decl_spec_node = is_declaration_specifiers()) {
        if (auto decl_node = is_declarator()) {
            auto decl_list_node = is_declaration_list();
            if (auto comp_node = is_compound_statement()) {
                next.push_back(std::move(decl_spec_node));
                next.push_back(std::move(decl_node));
                if (decl_list_node) next.push_back(std::move(decl_list_node));
                next.push_back(std::move(comp_node));
                return MkNd(FunctionDefinition);
            }
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::is_block_item() {
    if (auto decl_node = is_declaration()) {
        return decl_node;
    } else if (auto stat_node = is_statement()) {
        return stat_node;
    } else return nullptr;
}

ASTNodePtr Parser::is_type_specifier() {
    std::vector<ASTNodePtr> next;
    auto tok = get_token_type();
    if (advance_if(MATCH_ANY(
        TokenType::Void,
        TokenType::Int,
        TokenType::Char,
        TokenType::Double,
        TokenType::Short,
        TokenType::Long,
        TokenType::Float,
        TokenType::Signed,
        TokenType::Unsigned,
        TokenType::Complex,
        TokenType::Imaginary,
        TokenType::Bool
    )))
    {
        next.push_back(std::move(MakeNode(serialize_a(deserialize(tok)), {})));
        return MkNd(TypeSpecifier);
    } else if (auto struct_node = is_struct_or_union_specifier()) {
        return std::move(struct_node);
    } else if (auto enum_node = is_enum_specifier()) {
        return std::move(enum_node);
    } else if (auto typedef_node = is_typedef_name()) {
        return std::move(typedef_node);
    } else return nullptr;
}

ASTNodePtr Parser::is_function_specifier() {
    RETURN_IF_MATCH(FunctionSpecifier, TokenType::Inline);
}

ASTNodePtr Parser::is_identifier() {
    auto value = get_token_value();
    if (is_keyword(TokenType::Identifier)) {
        auto node = MakeNode(ASTNodeType::Identifier, {});
        node->Value = value;
        return std::move(node);
    }
    return nullptr;
}

ASTNodePtr Parser::is_function_arguments() {
    if (auto lpar_node = is_punctuator('(')) {
        if (auto arg_list_node = is_argument_list()) {
            if (auto rpar_node = is_punctuator(')')) {
                std::vector<ASTNodePtr> next;
                next.push_back(std::move(arg_list_node));
                return MkNd(FunctionArguments);
            }
        }
        parser_error();
    }
    return nullptr;
}

bool Parser::is_punctuator(char c) {
    return advance_if(get_token_type() == TokenType::Punctuator && get_token_value()[0] == c);
}

bool Parser::is_keyword(TokenType t) {
    return advance_if(get_token_type() == t);
}

ASTNodePtr Parser::is_storage_class_specifier() {
    RETURN_IF_MATCH(StorageClassSpecifier,
        TokenType::Typedef,
        TokenType::Extern,
        TokenType::Static,
        TokenType::Auto,
        TokenType::Register
    );
}

ASTNodePtr Parser::is_struct_or_union_specifier() {
    std::vector<ASTNodePtr> next;
    if (auto str_node = is_struct_or_union()) {
        auto id_node = is_identifier();
        if (is_punctuator('{')) {
            if (auto decl_list_node = is_struct_declaration_list()) {
                if (is_punctuator('}')) {
                    next.push_back(std::move(str_node));
                    if (id_node) next.push_back(std::move(id_node));
                    next.push_back(std::move(decl_list_node));
                    return MkNd(StructOrUnionSpecifier);
                }
            }
        } else if (id_node) {
            next.push_back(std::move(str_node));
            next.push_back(std::move(id_node));
            return MkNd(StructOrUnionSpecifier);
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::is_struct_declaration() {
    std::vector<ASTNodePtr> next;
    if (auto spec_node = is_specifier_qualifier_list()) {
        if (auto decl_list_node = is_struct_declarator_list()) {
            consume(';');
            next.push_back(std::move(spec_node));
            next.push_back(std::move(decl_list_node));
            return MkNd(StructDeclaration);
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::is_struct_declarator() {
    std::vector<ASTNodePtr> next;
    auto decl_node = is_declarator();
    if (is_punctuator(':')) {
        if (auto const_node = is_constant_expression()) {
            if (decl_node)
                next.push_back(std::move(decl_node));
            next.push_back(std::move(const_node));
        }
    } else if (decl_node) {
        next.push_back(std::move(decl_node));
    } else return nullptr;
    return MkNd(StructDeclarator);
}

ASTNodePtr Parser::is_constant_expression() {
    if (auto expr_node = is_conditional_expression()) {
        return expr_node;
    }
    return nullptr;
}

ASTNodePtr Parser::is_type_qualifier_list() {
    std::vector<ASTNodePtr> next;
    if (auto qual_node = is_type_qualifier()) {
        if (auto list_node = _is_type_qualifier_list()) {
            next.push_back(std::move(qual_node));
            next.push_back(std::move(list_node));
            return MkNd(TypeQualifierList);
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::_is_type_qualifier_list() {
    std::vector<ASTNodePtr> next;
    if (auto qual_node = is_type_qualifier()) {
        if (auto list_node = _is_type_qualifier_list()) {
            next.push_back(std::move(qual_node));
            next.push_back(std::move(list_node));
            return MkNd(TypeQualifierList);
        }
        parser_error();
    }
    return MkNd(TypeQualifierList);
}

ASTNodePtr Parser::is_declaration_list() {
    std::vector<ASTNodePtr> next;
    if (auto decl_node = is_declaration()) {
        if (auto list_node = _is_declaration_list()) {
            next.push_back(std::move(decl_node));
            next.push_back(std::move(list_node));
            return MkNd(DeclarationList);
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::_is_declaration_list() {
    std::vector<ASTNodePtr> next;
    if (auto decl_node = is_declaration()) {
        if (auto list_node = _is_declaration_list()) {
            next.push_back(std::move(decl_node));
            next.push_back(std::move(list_node));
            return MkNd(DeclarationList);
        }
        parser_error();
    }
    return MkNd(DeclarationList);
}

ASTNodePtr Parser::is_block_item_list() {
    std::vector<ASTNodePtr> next;
    if (auto item_node = is_block_item()) {
        if (auto list_node = _is_block_item_list()) {
            next.push_back(std::move(item_node));
            if (!list_node->IsEmpty()) next.push_back(std::move(list_node));
            return BlockItemListNode::Create(MkNd(BlockItemList));
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::_is_block_item_list() {
    std::vector<ASTNodePtr> next;
    if (auto item_node = is_block_item()) {
        if (auto list_node = _is_block_item_list()) {
            next.push_back(std::move(item_node));
            if (!list_node->IsEmpty()) next.push_back(std::move(list_node));
            return MkNd(BlockItemList);
        }
        parser_error();
    }
    return MkNd(BlockItemList);
}

ASTNodePtr Parser::is_struct_declaration_list() {
    std::vector<ASTNodePtr> next;
    if (auto decl_node = is_struct_declaration()) {
        if (auto list_node = _is_struct_declaration_list()) {
            next.push_back(std::move(decl_node));
            if (!list_node->IsEmpty()) next.push_back(std::move(list_node));
            return StructDeclarationListNode::Create(MkNd(StructDeclaratorList));
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::_is_struct_declaration_list() {
    std::vector<ASTNodePtr> next;
    if (auto decl_node = is_struct_declaration()) {
        if (auto list_node = _is_struct_declaration_list()) {
            next.push_back(std::move(decl_node));
            if (!list_node->IsEmpty()) next.push_back(std::move(list_node));
            return MkNd(StructDeclarationList);
        }
        parser_error();
    }
    return MkNd(StructDeclarationList);
}

ASTNodePtr Parser::is_struct_declarator_list() {
    std::vector<ASTNodePtr> next;
    if (auto decl_node = is_struct_declarator()) {
        if (auto list_node = _is_struct_declarator_list()) {
            next.push_back(std::move(decl_node));
            if (!list_node->IsEmpty()) next.push_back(std::move(list_node));
            return StructDeclaratorListNode::Create(MkNd(StructDeclaratorList));
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::_is_struct_declarator_list() {
    std::vector<ASTNodePtr> next;
    if (is_punctuator(',')) {
        if (auto decl_node = is_struct_declarator()) {
            if (auto list_node = _is_struct_declarator_list()) {
                next.push_back(std::move(decl_node));
                if (!list_node->IsEmpty()) next.push_back(std::move(list_node));
                return MkNd(StructDeclaratorList);
            }
        }
        parser_error();
    }
    return MkNd(StructDeclaratorList);
}

ASTNodePtr Parser::is_init_declarator_list() {
    std::vector<ASTNodePtr> next;
    if (auto decl_node = is_init_declarator()) {
        if (auto list_node = _is_init_declarator_list()) {
            next.push_back(std::move(decl_node));
            next.push_back(std::move(list_node));
            return MkNd(InitDeclaratorList);
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::_is_init_declarator_list() {
    std::vector<ASTNodePtr> next;
    if (is_punctuator(',')) {
        if (auto decl_node = is_init_declarator()) {
            if (auto list_node = _is_init_declarator_list()) {
                next.push_back(std::move(decl_node));
                next.push_back(std::move(list_node));
                return MkNd(InitDeclaratorList);
            }
        }
        parser_error();
    }
    return MkNd(InitDeclaratorList);
}

ASTNodePtr Parser::is_parameter_list() {
    std::vector<ASTNodePtr> next;
    if (auto param_node = is_parameter_declaration()) {
        if (auto list_node = _is_parameter_list()) {
            next.push_back(std::move(param_node));
            next.push_back(std::move(list_node));
            return MkNd(ParameterList);
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::_is_parameter_list() {
    std::vector<ASTNodePtr> next;
    if (is_punctuator(',')) {
        if (auto param_node = is_parameter_declaration()) {
            if (auto list_node = _is_parameter_list()) {
                next.push_back(std::move(param_node));
                next.push_back(std::move(list_node));
                return MkNd(ParameterList);
            }
        } else {
            // Unconsume the comma
            // Because parameter_type_list will consume it
            --index_;
        }
    }
    return MkNd(ParameterList);
}

ASTNodePtr Parser::is_identifier_list() {
    std::vector<ASTNodePtr> next;
    if (auto id = is_identifier()) {
        if (auto list_node = _is_identifier_list()) {
            next.push_back(std::move(id));
            next.push_back(std::move(list_node));
            return MkNd(IdentifierList);
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::_is_identifier_list() {
    std::vector<ASTNodePtr> next;
    if (is_punctuator(',')) {
        if (auto id = is_identifier()) {
            if (auto list_node = _is_identifier_list()) {
                next.push_back(std::move(id));
                next.push_back(std::move(list_node));
                return MkNd(IdentifierList);
            }
        }
        parser_error();
    }
    return MkNd(IdentifierList);
}

ASTNodePtr Parser::is_shift_expression() {
    std::vector<ASTNodePtr> next;
    if (auto expr_node = is_additive_expression()) {
        if (auto expr_node2 = _is_shift_expression()) {
            if (expr_node2->IsEmpty()) return expr_node;
            next.push_back(std::move(expr_node));
            next.push_back(std::move(expr_node2));
            return ShiftExpressionNode::Create(MkNd(ShiftExpression));
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::_is_shift_expression() {
    std::vector<ASTNodePtr> next;
    if (is_keyword(TokenType::LeftOp) || is_keyword(TokenType::RightOp)) {
        if (auto expr_node = is_additive_expression()) {
            if (auto expr_node2 = _is_shift_expression()) {
                next.push_back(std::move(expr_node));
                if (!expr_node2->IsEmpty()) next.push_back(std::move(expr_node2));
                return MkNd(ShiftExpression);
            }
        }
        parser_error();
    }
    return MkNd(ShiftExpression);
}

ASTNodePtr Parser::is_additive_expression() {
    std::vector<ASTNodePtr> next;
    if (auto expr_node = is_multiplicative_expression()) {
        if (auto expr_node2 = _is_additive_expression()) {
            if (expr_node2->IsEmpty()) return expr_node;
            next.push_back(std::move(expr_node));
            next.push_back(std::move(expr_node2));
            return AdditiveExpressionNode::Create(MkNd(AdditiveExpression));
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::_is_additive_expression() {
    std::vector<ASTNodePtr> next;
    if (is_punctuator('+') || is_punctuator('-')) {
        if (auto expr_node = is_multiplicative_expression()) {
            if (auto expr_node2 = _is_additive_expression()) {
                next.push_back(std::move(expr_node));
                if (!expr_node2->IsEmpty()) next.push_back(std::move(expr_node2));
                return MkNd(AdditiveExpression);
            }
        }
        parser_error();
    }
    return MkNd(AdditiveExpression);
}

ASTNodePtr Parser::is_multiplicative_expression() {
    std::vector<ASTNodePtr> next;
    if (auto expr_node = is_cast_expression()) {
        if (auto expr_node2 = _is_multiplicative_expression()) {
            if (expr_node2->IsEmpty()) return expr_node;
            next.push_back(std::move(expr_node));
            next.push_back(std::move(expr_node2));
            return MultiplicativeExpressionNode::Create(MkNd(MultiplicativeExpression));
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::_is_multiplicative_expression() {
    std::vector<ASTNodePtr> next;
    if (is_punctuator('*') || is_punctuator('/') || is_punctuator('%')) {
        if (auto expr_node = is_cast_expression()) {
            if (auto expr_node2 = _is_multiplicative_expression()) {
                next.push_back(std::move(expr_node));
                if (!expr_node2->IsEmpty()) next.push_back(std::move(expr_node2));
                return MkNd(MultiplicativeExpression);
            }
        }
        parser_error();
    }
    return MkNd(MultiplicativeExpression);
}

ASTNodePtr Parser::is_argument_expression_list() {
    std::vector<ASTNodePtr> next;
    if (auto expr_node = is_assignment_expression()) {
        if (auto list_node = _is_argument_expression_list()) {
            next.push_back(std::move(expr_node));
            next.push_back(std::move(list_node));
            return MkNd(ArgumentExpressionList);
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::_is_argument_expression_list() {
    std::vector<ASTNodePtr> next;
    if (is_punctuator(',')) {
        if (auto expr_node = is_assignment_expression()) {
            if (auto list_node = _is_argument_expression_list()) {
                next.push_back(std::move(expr_node));
                next.push_back(std::move(list_node));
                return MkNd(ArgumentExpressionList);
            }
        }
        parser_error();
    }
    return MkNd(ArgumentExpressionList);
}

ASTNodePtr Parser::is_enumerator_list(){
    std::vector<ASTNodePtr> next;
    if (auto enum_node = is_enumerator()) {
        if (auto list_node = _is_enumerator_list()) {
            next.push_back(std::move(enum_node));
            next.push_back(std::move(list_node));
            return MkNd(EnumeratorList);
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::_is_enumerator_list(){
    std::vector<ASTNodePtr> next;
    if (is_punctuator(',')) {
        if (auto enum_node = is_enumerator()) {
            if (auto list_node = _is_enumerator_list()) {
                next.push_back(std::move(enum_node));
                next.push_back(std::move(list_node));
                return MkNd(EnumeratorList);
            }
        }
        parser_error();
    }
    return MkNd(EnumeratorList);
}

ASTNodePtr Parser::is_initializer_list() {
    std::vector<ASTNodePtr> next;
    if (auto desi_node = is_designation()) {
        if (auto init_node = is_initializer()) {
            if (auto list_node = _is_initializer_list()) {
                next.push_back(std::move(init_node));
                next.push_back(std::move(list_node));
                return MkNd(InitializerList);
            }
        }
        parser_error();
    } else if (auto init_node = is_initializer()) {
        if (auto list_node = _is_initializer_list()) {
            next.push_back(std::move(init_node));
            next.push_back(std::move(list_node));
            return MkNd(InitializerList);
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::_is_initializer_list() {
    std::vector<ASTNodePtr> next;
    if (is_punctuator(',')) {
        if (auto desi_node = is_designation()) {
            if (auto init_node = is_initializer()) {
                if (auto list_node = _is_initializer_list()) {
                    next.push_back(std::move(init_node));
                    next.push_back(std::move(list_node));
                    return MkNd(InitializerList);
                }
            }
        } else if (auto init_node = is_initializer()) {
            if (auto list_node = _is_initializer_list()) {
                next.push_back(std::move(init_node));
                next.push_back(std::move(list_node));
                return MkNd(InitializerList);
            }
        }
        parser_error();
    }
    return MkNd(InitializerList);
}

ASTNodePtr Parser::is_designator_list() {
    std::vector<ASTNodePtr> next;
    if (auto desi_node = is_designator()) {
        if (auto list_node = _is_designator_list()) {
            next.push_back(std::move(desi_node));
            next.push_back(std::move(list_node));
            return MkNd(DesignatorList);
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::_is_designator_list() {
    std::vector<ASTNodePtr> next;
    if (auto desi_node = is_designator()) {
        if (auto list_node = _is_designator_list()) {
            next.push_back(std::move(desi_node));
            next.push_back(std::move(list_node));
            return MkNd(DesignatorList);
        }
        parser_error();
    }
    return MkNd(DesignatorList);
}

ASTNodePtr Parser::is_expression() {
    std::vector<ASTNodePtr> next;
    if (auto expr_node = is_assignment_expression()) {
        if (auto list_node = _is_expression()) {
            if (list_node->IsEmpty()) return expr_node;
            next.push_back(std::move(expr_node));
            next.push_back(std::move(list_node));
            return MkNd(Expression);
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::_is_expression() {
    std::vector<ASTNodePtr> next;
    if (is_punctuator(',')) {
        if (auto expr_node = is_assignment_expression()) {
            if (auto list_node = _is_expression()) {
                if (list_node->IsEmpty()) return expr_node;
                next.push_back(std::move(expr_node));
                next.push_back(std::move(list_node));
                return MkNd(Expression);
            }
        }
        parser_error();
    }
    return MkNd(Expression);
}

ASTNodePtr Parser::is_direct_declarator() {
    std::vector<ASTNodePtr> next;
    if (auto id = is_identifier()) {
        if (auto list_node = _is_direct_declarator()) {
            if (list_node->IsEmpty()) return id;
            next.push_back(std::move(id));
            next.push_back(std::move(list_node));
            return MkNd(DirectDeclarator);
        }
    } else if (is_punctuator('(')) {
        if (auto decl_node = is_declarator()) {
            consume(')');
            if (auto list_node = _is_direct_declarator()) {
                if (list_node->IsEmpty()) return decl_node;
                next.push_back(std::move(decl_node));
                next.push_back(std::move(list_node));
                return MkNd(DirectDeclarator);
            }
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::_is_direct_declarator() {
    std::vector<ASTNodePtr> next;
    if (is_punctuator('[')) {
        auto type_node = is_type_qualifier_list();
        auto assi_node = is_assignment_expression();
        if (is_punctuator(']')) {
            if (type_node) next.push_back(std::move(type_node));
            if (assi_node) next.push_back(std::move(assi_node));
            auto decl_node = _is_direct_declarator();
            if (decl_node && !decl_node->IsEmpty()) next.push_back(std::move(decl_node));
            return MkNd(DirectDeclarator);
        } else if (is_punctuator('*')) {
            if (type_node) next.push_back(std::move(type_node));
            auto decl_node = _is_direct_declarator();
            if (decl_node && !decl_node->IsEmpty()) next.push_back(std::move(decl_node));
            return MkNd(DirectDeclarator);
        } else if (is_keyword(TokenType::Static)) {
            if (!type_node) type_node = is_type_qualifier_list();
            if (assi_node = is_assignment_expression()) {
                if (is_punctuator(']')) {
                    next.push_back(std::move(type_node));
                    next.push_back(std::move(assi_node));
                    auto decl_node = _is_direct_declarator();
                    if (decl_node && !decl_node->IsEmpty()) next.push_back(std::move(decl_node));
                    return MkNd(DirectDeclarator);
                }
            }
            parser_error();
        }
    } else if (is_punctuator('(')) {
        if (auto param_node = is_parameter_type_list()) {
            consume(')');
            if (auto list_node = _is_direct_declarator()) {
                next.push_back(std::move(param_node));
                if (!list_node->IsEmpty())next.push_back(std::move(list_node));
                return MkNd(DirectDeclarator);
            }
        } else {
            auto id_list_node = is_identifier_list();
            if (is_punctuator(')')) {
                if (auto list_node = _is_direct_declarator()) {
                    if (id_list_node) next.push_back(std::move(id_list_node));
                    if (!list_node->IsEmpty()) next.push_back(std::move(list_node));
                    return MkNd(DirectDeclarator);
                }
            }
        }
    }
    return MkNd(DirectDeclarator);
}

ASTNodePtr Parser::is_relational_expression() {
    std::vector<ASTNodePtr> next;
    if (auto shift_node = is_shift_expression()) {
        if (auto rel_node = _is_relational_expression()) {
            if (rel_node->IsEmpty()) return shift_node;
            next.push_back(std::move(shift_node));
            next.push_back(std::move(rel_node));
            return RelationalExpressionNode::Create(MkNd(RelationalExpression));
        }
    }
    return nullptr;
}

ASTNodePtr Parser::_is_relational_expression() {
    std::vector<ASTNodePtr> next;
    if (is_keyword(TokenType::LeOp) || is_keyword(TokenType::GeOp) || is_punctuator('<') || is_punctuator('>')) {
        if (auto shift_node = is_shift_expression()) {
            if (auto rel_node = _is_relational_expression()) {
                next.push_back(std::move(shift_node));
                if (!rel_node->IsEmpty()) next.push_back(std::move(rel_node));
                return MkNd(RelationalExpression);
            }
        }
        parser_error();
    }
    return MkNd(RelationalExpression);
}

ASTNodePtr Parser::is_equality_expression() {
    std::vector<ASTNodePtr> next;
    if (auto rel_node = is_relational_expression()) {
        if (auto eq_node = _is_equality_expression()) {
            if (eq_node->IsEmpty()) return rel_node;
            next.push_back(std::move(rel_node));
            next.push_back(std::move(eq_node));
            return EqualityExpressionNode::Create(MkNd(EqualityExpression));
        }
    }
    return nullptr;
}

ASTNodePtr Parser::_is_equality_expression() {
    std::vector<ASTNodePtr> next;
    if (is_keyword(TokenType::EqOp) || is_keyword(TokenType::NeOp)) {
        if (auto rel_node = is_relational_expression()) {
            if (auto eq_node = _is_equality_expression()) {
                next.push_back(std::move(rel_node));
                if (!eq_node->IsEmpty()) next.push_back(std::move(eq_node));
                return MkNd(EqualityExpression);
            }
        }
        parser_error();
    }
    return MkNd(EqualityExpression);
}

ASTNodePtr Parser::is_and_expression() {
    std::vector<ASTNodePtr> next;
    if (auto eq_node = is_equality_expression()) {
        if (auto and_node = _is_and_expression()) {
            if (and_node->IsEmpty()) return eq_node;
            next.push_back(std::move(eq_node));
            next.push_back(std::move(and_node));
            return AndExpressionNode::Create(MkNd(AndExpression));
        }
    }
    return nullptr;
}

ASTNodePtr Parser::_is_and_expression() {
    std::vector<ASTNodePtr> next;
    if (is_punctuator('&')) {
        if (auto eq_node = is_equality_expression()) {
            if (auto and_node = _is_and_expression()) {
                next.push_back(std::move(eq_node));
                if (!and_node->IsEmpty()) next.push_back(std::move(and_node));
                return MkNd(AndExpression);
            }
        }
        parser_error();
    }
    return MkNd(AndExpression);
}

ASTNodePtr Parser::is_xor_expression() {
    std::vector<ASTNodePtr> next;
    if (auto and_node = is_and_expression()) {
        if (auto xor_node = _is_xor_expression()) {
            if (xor_node->IsEmpty()) return and_node;
            next.push_back(std::move(and_node));
            next.push_back(std::move(xor_node));
            return XorExpressionNode::Create(MkNd(ExclusiveOrExpression));
        }
    }
    return nullptr;
}

ASTNodePtr Parser::_is_xor_expression() {
    std::vector<ASTNodePtr> next;
    if (is_punctuator('^')) {
        if (auto and_node = is_and_expression()) {
            if (auto xor_node = _is_xor_expression()) {
                next.push_back(std::move(and_node));
                if (!xor_node->IsEmpty()) next.push_back(std::move(xor_node));
                return MkNd(ExclusiveOrExpression);
            }
        }
        parser_error();
    }
    return MkNd(ExclusiveOrExpression);
}

ASTNodePtr Parser::is_or_expression() {
    std::vector<ASTNodePtr> next;
    if (auto xor_node = is_xor_expression()) {
        if (auto or_node = _is_or_expression()) {
            if (or_node->IsEmpty()) return xor_node;
            next.push_back(std::move(xor_node));
            next.push_back(std::move(or_node));
            return OrExpressionNode::Create(MkNd(InclusiveOrExpression));
        }
    }
    return nullptr;
}

ASTNodePtr Parser::_is_or_expression() {
    std::vector<ASTNodePtr> next;
    if (is_punctuator('|')) {
        if (auto xor_node = is_xor_expression()) {
            if (auto or_node = _is_or_expression()) {
                next.push_back(std::move(xor_node));
                if (!or_node->IsEmpty()) next.push_back(std::move(or_node));
                return MkNd(InclusiveOrExpression);
            }
        }
        parser_error();
    }
    return MkNd(InclusiveOrExpression);
}

ASTNodePtr Parser::is_logical_and_expression() {
    std::vector<ASTNodePtr> next;
    if (auto or_node = is_or_expression()) {
        if (auto and_node = _is_logical_and_expression()) {
            if (and_node->IsEmpty()) return or_node;
            next.push_back(std::move(or_node));
            next.push_back(std::move(and_node));
            return LogicalAndExpressionNode::Create(MkNd(LogicalAndExpression));
        }
    }
    return nullptr;
}

ASTNodePtr Parser::_is_logical_and_expression() {
    std::vector<ASTNodePtr> next;
    if (is_keyword(TokenType::AndOp)) {
        if (auto or_node = is_or_expression()) {
            if (auto and_node = _is_logical_and_expression()) {
                next.push_back(std::move(or_node));
                if (!and_node->IsEmpty()) next.push_back(std::move(and_node));
                return MkNd(LogicalAndExpression);
            }
        }
        parser_error();
    }
    return MkNd(LogicalAndExpression);
}

ASTNodePtr Parser::is_logical_or_expression() {
    std::vector<ASTNodePtr> next;
    if (auto and_node = is_logical_and_expression()) {
        if (auto or_node = _is_logical_or_expression()) {
            if (or_node->IsEmpty()) return and_node;
            next.push_back(std::move(and_node));
            next.push_back(std::move(or_node));
            return LogicalOrExpressionNode::Create(MkNd(LogicalOrExpression));
        }
    }
    return nullptr;
}

ASTNodePtr Parser::_is_logical_or_expression() {
    std::vector<ASTNodePtr> next;
    if (is_keyword(TokenType::OrOp)) {
        if (auto and_node = is_logical_and_expression()) {
            if (auto or_node = _is_logical_or_expression()) {
                next.push_back(std::move(and_node));
                if (!or_node->IsEmpty()) next.push_back(std::move(or_node));
                return MkNd(LogicalOrExpression);
            }
        }
        parser_error();
    }
    return MkNd(LogicalOrExpression);
}

ASTNodePtr Parser::is_postfix_expression() {
    std::vector<ASTNodePtr> next;
    if (auto pr_node = is_primary_expression()) {
        if (auto pr_node2 = _is_postfix_expression()) {
            if (pr_node2->IsEmpty()) return pr_node;
            next.push_back(std::move(pr_node));
            next.push_back(std::move(pr_node2));
            return MkNd(PostfixExpression);
        }
    } else if (is_punctuator('(')) {
        if (auto type_node = is_type_name()) {
            consume(')');
            consume('{');
            if (auto init_node = is_initializer_list()) {
                is_punctuator(','); // Optional comma
                consume('}');
                next.push_back(std::move(type_node));
                next.push_back(std::move(init_node));
                if (auto pr_node2 = _is_postfix_expression()) {
                    if (!pr_node2->IsEmpty()) next.push_back(std::move(pr_node2));
                    return MkNd(PostfixExpression);
                }
            }
        }
    }
    return nullptr;
}

ASTNodePtr Parser::_is_postfix_expression() {
    std::vector<ASTNodePtr> next;
    if (is_punctuator('[')) {
        if (auto expr_node = is_expression()) {
            consume(']');
            if (auto pr_node2 = _is_postfix_expression()) {
                next.push_back(std::move(expr_node));
                if (!pr_node2->IsEmpty()) next.push_back(std::move(pr_node2));
                return MkNd(PostfixExpression);
            }
        }
        parser_error();
    } else if (is_punctuator('(')) {
        auto arg_list_node = is_argument_expression_list();
        consume(')');
        if (auto pr_node2 = _is_postfix_expression()) {
            next.push_back(std::move(arg_list_node));
            if (!pr_node2->IsEmpty()) next.push_back(std::move(pr_node2));
            return MkNd(PostfixExpression);
        }
    } else if (is_punctuator('.') || is_keyword(TokenType::PtrOp)) {
        if (auto id_node = is_identifier()) {
            if (auto pr_node2 = _is_postfix_expression()) {
                next.push_back(std::move(id_node));
                if (!pr_node2->IsEmpty()) next.push_back(std::move(pr_node2));
                return MkNd(PostfixExpression);
            }
        }
        parser_error();
    } else if (is_keyword(TokenType::IncOp) || is_keyword(TokenType::DecOp)) {
        if (auto pr_node2 = _is_postfix_expression()) {
            if (!pr_node2->IsEmpty()) next.push_back(std::move(pr_node2));
            return MkNd(PostfixExpression);
        }
        parser_error();
    }    
    return MkNd(PostfixExpression);
}

ASTNodePtr Parser::is_designator() {
    std::vector<ASTNodePtr> next;
    if (is_punctuator('[')) {
        if (auto expr_node = is_constant_expression()) {
            consume(']');
            next.push_back(std::move(expr_node));
            return MkNd(Designator);
        }
        parser_error();
    } else if (is_punctuator('.')) {
        if (auto id_node = is_identifier()) {
            next.push_back(std::move(id_node));
            return MkNd(Designator);
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::is_designation() {
    std::vector<ASTNodePtr> next;
    if (auto desi_node = is_designator_list()) {
        consume('=');
        next.push_back(std::move(desi_node));
        return MkNd(Designation);
    }
    return nullptr;
}

ASTNodePtr Parser::is_cast_expression() {
    std::vector<ASTNodePtr> next;
    if (is_punctuator('(')) {
        if (auto type_node = is_type_name()) {
            if (is_punctuator(')')) {
                if (auto cast_node = is_cast_expression()) {
                    next.push_back(std::move(type_node));
                    next.push_back(std::move(cast_node));
                    return MkNd(CastExpression);
                }
            }
        }
    } else if (auto un_node = is_unary_expression()) {
        return un_node;
    }
    return nullptr;
}

ASTNodePtr Parser::is_unary_expression() {
    std::vector<ASTNodePtr> next;
    if (auto post_node = is_postfix_expression()) {
        return post_node;
    } else if (is_keyword(TokenType::IncOp) || is_keyword(TokenType::DecOp)) {
        if (auto un_node = is_unary_expression()) {
            next.push_back(std::move(un_node));
            return MkNd(UnaryExpression);
        }
        parser_error();
    } else if (auto un_node = is_unary_operator()) {
        if (auto cast_node = is_cast_expression()) {
            next.push_back(std::move(un_node));
            next.push_back(std::move(cast_node));
            return MkNd(UnaryExpression);
        }
        parser_error();
    } else if (is_keyword(TokenType::Sizeof)) {
        if (auto unex_node = is_unary_expression()) {
            next.push_back(std::move(unex_node));
            return MkNd(UnaryExpression);
        } else if (is_punctuator('(')) {
            if (auto type_node = is_type_name()) {
                if (is_punctuator(')')) {
                    next.push_back(std::move(type_node));
                    return MkNd(UnaryExpression);
                }
            }
        }
    }
    return nullptr;
}

ASTNodePtr Parser::is_enumerator() {
    std::vector<ASTNodePtr> next;
    if (auto enum_node = is_enumeration_constant()) {
        if (is_punctuator('=')) {
            if (auto expr_node = is_constant_expression()) {
                next.push_back(std::move(enum_node));
                next.push_back(std::move(expr_node));
                return MkNd(Enumerator);
            }
        } else {
            next.push_back(std::move(enum_node));
            return MkNd(Enumerator);
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::is_enumeration_constant() {
    std::vector<ASTNodePtr> next;
    if (auto id = is_identifier()) {
        next.push_back(std::move(id));
        return MkNd(EnumerationConstant);
    }
    return nullptr;
}

ASTNodePtr Parser::is_argument_list() {
    std::vector<ASTNodePtr> next;
    if (auto arg_node = is_argument()) {
        next.push_back(std::move(arg_node));
        auto args_node = MkNd(ArgumentList);
        if (auto comma_node = is_punctuator(',')) {
            if (auto args_node2 = is_argument_list()) {
                for (auto& arg : args_node2->Next) {
                    args_node->Next.push_back(std::move(arg));
                }
                return args_node;
            } else {
                parser_error();
            }
        } else {
            return args_node;
        }
    }
    return MkNd(ArgumentList);
}

ASTNodePtr Parser::is_argument() {
    if (auto type_node = is_type_specifier()) {
        auto id_node = is_identifier();
        std::vector<ASTNodePtr> next;
        next.push_back(std::move(type_node));
        next.push_back(std::move(id_node));
        return MkNd(Argument);
    }
    return nullptr;
}

ASTNodePtr Parser::is_declaration() {
    if (auto decl_node = is_declaration_specifiers()) {
        std::vector<ASTNodePtr> next;
        auto init_node = is_init_declarator_list();
        consume(';');
        if (init_node) next.push_back(std::move(init_node));
        next.push_back(std::move(decl_node));
        return MkNd(Declaration);
    }
    return nullptr;
}

ASTNodePtr Parser::is_primary_expression() {
    if (auto id_node = is_identifier()) {
        return id_node;
    } else if (auto num_node = is_constant()) {
        return num_node;
    } else if (auto str_node = is_string_literal()) {
        return str_node;
    } else if (is_punctuator('(')) {
        if (auto expr_node = is_expression()) {
            consume(')');
            return expr_node;
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::is_string_literal() {
    RETURN_IF_MATCH(StringLiteral, TokenType::StringLiteral);
}

ASTNodePtr Parser::is_constant() {
    RETURN_IF_MATCH(
        Constant,
        TokenType::IntegerConstant,
        TokenType::FloatingConstant,
        TokenType::EnumerationConstant,
        TokenType::CharacterConstant,
        TokenType::OctalConstant,
        TokenType::HexadecimalConstant,
    );
}

ASTNodePtr Parser::is_declaration_specifiers() {
    // TODO unfinished
    std::vector<ASTNodePtr> next;
    if  (auto storage_node = is_storage_class_specifier()) {
        auto decl_node = is_declaration_specifiers();
        if (decl_node) next.push_back(std::move(decl_node));
        next.push_back(std::move(storage_node));
        return MkNd(DeclarationSpecifiers);
    } else if (auto types_node = is_type_specifier()) {
        auto decl_node = is_declaration_specifiers();
        if (decl_node) next.push_back(std::move(decl_node));
        next.push_back(std::move(types_node));
        return MkNd(DeclarationSpecifiers);
    } else if (auto typeq_node = is_type_qualifier()) {
        auto decl_node = is_declaration_specifiers();
        if (decl_node) next.push_back(std::move(decl_node));
        next.push_back(std::move(typeq_node));
        return MkNd(DeclarationSpecifiers);
    } else if (auto spec_node = is_function_specifier()) {
        auto decl_node = is_declaration_specifiers();
        if (decl_node) next.push_back(std::move(decl_node));
        next.push_back(std::move(spec_node));
        return MkNd(DeclarationSpecifiers);
    }
    return nullptr;
}

ASTNodePtr Parser::is_statement_list() {
    std::vector<ASTNodePtr> next;
    return MkNd(StatementList);
}

ASTNodePtr Parser::is_statement() {
    if (auto l_stat_node = is_labeled_statement()) {
        return l_stat_node;
    } else if (auto c_stat_node = is_compound_statement()) {
        return c_stat_node;
    } else if (auto e_stat_node = is_expression_statement()) {
        return e_stat_node;
    } else if (auto s_stat_node = is_selection_statement()) {
        return s_stat_node;
    } else if (auto i_stat_node = is_iteration_statement()) {
        return i_stat_node;
    } else if (auto j_stat_node = is_jump_statement()) {
        return j_stat_node;
    } else return nullptr;
}

ASTNodePtr Parser::is_selection_statement() {
    std::vector<ASTNodePtr> next;
    if (is_keyword(TokenType::If)) {
        consume('(');
        if (auto expr_node = is_expression()) {
            consume(')');
            if (auto stat_node = is_statement()) {
                next.push_back(std::move(expr_node));
                next.push_back(std::move(stat_node));
                if (is_keyword(TokenType::Else)) {
                    if (auto stat_else_node = is_statement()) {
                        next.push_back(std::move(stat_else_node));
                        return MkNd(SelectionStatement);
                    }
                } else {
                    return MkNd(SelectionStatement);
                }
            }
        }
        parser_error();
    } else if (is_keyword(TokenType::Switch)) {
        consume('(');
        if (auto expr_node = is_expression()) {
            consume(')');
            if (auto stat_node = is_statement()) {
                next.push_back(std::move(expr_node));
                next.push_back(std::move(stat_node));
                return MkNd(SelectionStatement);
            }
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::is_iteration_statement() {
    std::vector<ASTNodePtr> next;
    if (is_keyword(TokenType::While)) {
        consume('(');
        if (auto expr_node = is_expression()) {
            consume(')');
            if (auto stat_node = is_statement()) {
                next.push_back(std::move(expr_node));
                next.push_back(std::move(stat_node));
                return MkNd(IterationStatement);
            }
        }
        parser_error();
    } else if (is_keyword(TokenType::Do)) {
        if (auto stat_node = is_statement()) {
            consume(TokenType::While);
            consume('(');
            if (auto expr_node = is_expression()) {
                consume(')');
                consume(';');
                next.push_back(std::move(expr_node));
                next.push_back(std::move(stat_node));
                return MkNd(IterationStatement);
            }
        }
        parser_error();
    } else if (is_keyword(TokenType::For)) {
        consume('(');
        if (auto expr_node = is_expression()) {
            consume(';');
            auto expr2_node = is_expression();
            consume(';');
            auto expr3_node = is_expression();
            consume(')');
            if (auto stat_node = is_statement()) {
                if (expr_node) next.push_back(std::move(expr_node));
                if (expr2_node) next.push_back(std::move(expr2_node));
                if (expr3_node) next.push_back(std::move(expr3_node));
                next.push_back(std::move(stat_node));
                return MkNd(IterationStatement);
            }
        } else if (auto decl_node = is_declaration()) {
            auto expr2_node = is_expression();
            consume(';');
            auto expr3_node = is_expression();
            consume(')');
            if (auto stat_node = is_statement()) {
                next.push_back(std::move(decl_node));
                if (expr2_node) next.push_back(std::move(expr2_node));
                if (expr3_node) next.push_back(std::move(expr3_node));
                next.push_back(std::move(stat_node));
                return MkNd(IterationStatement);
            }
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::is_jump_statement() {
    std::vector<ASTNodePtr> next;
    if (is_keyword(TokenType::Goto)) {
        if (auto id = is_identifier()) {
            consume(';');
            next.push_back(std::move(MakeNode(ASTNodeType::Goto, {})));
            next.push_back(std::move(id));
            return MkNd(JumpStatement);
        }
        parser_error();
    } else if (is_keyword(TokenType::Continue)) {
        consume(';');
        next.push_back(std::move(MakeNode(ASTNodeType::Continue, {})));
        return MkNd(JumpStatement);
    } else if (is_keyword(TokenType::Break)) {
        consume(';');
        next.push_back(std::move(MakeNode(ASTNodeType::Break, {})));
        return MkNd(JumpStatement);
    } else if (is_keyword(TokenType::Return)) {
        auto expr_node = is_expression();
        consume(';');
        next.push_back(std::move(MakeNode(ASTNodeType::Return, {})));
        if (expr_node) next.push_back(std::move(expr_node));
        return MkNd(JumpStatement);
    }
    return nullptr;
}

ASTNodePtr Parser::is_expression_statement() {
    auto expr_node = is_expression();
    if (expr_node) {
        consume(';');
        return expr_node;
    } else if (is_punctuator(';')) {
        std::vector<ASTNodePtr> next;
        next.push_back(std::move(expr_node));
        return MkNd(Expression);
    }
    return nullptr;
}

ASTNodePtr Parser::is_labeled_statement() {
    std::vector<ASTNodePtr> next;
    if (auto id = is_identifier()) {
        if (is_punctuator(':')) {
            if (auto stat_node = is_statement()) {
                next.push_back(std::move(id));
                next.push_back(std::move(stat_node));
                return MkNd(LabeledStatement);
            }
        } else {
            --index_;
            return nullptr;
        }
        parser_error();
    } else if (is_keyword(TokenType::Case)) {
        if (auto const_node = is_constant_expression()) {
            consume(':');
            if (auto stat_node = is_statement()) {
                next.push_back(std::move(const_node));
                next.push_back(std::move(stat_node));
                return MkNd(LabeledStatement);
            }
        }
        parser_error();
    } else if (is_keyword(TokenType::Default)) {
        consume(':');
        if (auto stat_node = is_statement()) {
            next.push_back(std::move(stat_node));
            return MkNd(LabeledStatement);
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::is_compound_statement() {
    if (is_punctuator('{')) {
        std::vector<ASTNodePtr> next;
        auto block_list = is_block_item_list();
        consume('}');
        if (block_list) next.push_back(std::move(block_list));
        return MkNd(CompoundStatement);
    }
    return nullptr;
}

ASTNodePtr Parser::is_assignment_expression() {
    auto bk_index = index_;
    if (auto un_node = is_conditional_expression()) {
        auto op_type = get_token_type();
        if (auto assi_oper_node = is_assignment_operator()) {
            // TODO: conditional_expression must be lvalue, otherwise error
            if (auto assi_node = is_assignment_expression()) {
                return ModifyExpressionNode::Create(std::move(un_node), std::move(assi_node), TokenType::Error, TokenType::Error, op_type);
            }
            parser_error();
        }
        return un_node;
    }
    return nullptr;
}

ASTNodePtr Parser::is_conditional_expression() {
    std::vector<ASTNodePtr> next;
    if (auto or_node = is_logical_or_expression()) {
        if (is_punctuator('?')) {
            if (auto expr_node = is_expression()) {
                if (is_punctuator(':')) {
                    if (auto cond_node = is_conditional_expression()) {
                        next.push_back(std::move(or_node));
                        next.push_back(std::move(expr_node));
                        next.push_back(std::move(cond_node));
                        return MkNd(ConditionalExpression);
                    }
                }
            }
            parser_error();
        }
        return or_node;
    }
    return nullptr;
}

ASTNodePtr Parser::is_struct_or_union() {
    RETURN_IF_MATCH(StructOrUnion, TokenType::Struct, TokenType::Union);
}

ASTNodePtr Parser::is_type_qualifier() {
    RETURN_IF_MATCH(TypeQualifier, TokenType::Const, TokenType::Volatile, TokenType::Restrict);
}

ASTNodePtr Parser::is_enum_specifier() {
    return nullptr;
}

ASTNodePtr Parser::is_typedef_name() {
    return nullptr;
    std::vector<ASTNodePtr> next;
    if (get_token_type() == TokenType::Identifier) {
        auto val = get_token_value();
        if (type_defined(val)) {
            advance_if(true);
            auto id = MakeNode(ASTNodeType::Identifier, {});
            id->Value = val;
            next.push_back(std::move(id));
            return MkNd(TypedefName);
        }
    }
    return nullptr;
}

ASTNodePtr Parser::is_declarator() {
    std::vector<ASTNodePtr> next;
    if (auto ptr_node = is_pointer()) {
        if (auto dir_node = is_direct_declarator()) {
            next.push_back(std::move(ptr_node));
            next.push_back(std::move(dir_node));
            return MkNd(Declarator);
        }
        parser_error();
    } else if (auto dir_node = is_direct_declarator()) {
        return dir_node;
    }
    return nullptr;
}

ASTNodePtr Parser::is_init_declarator() {
    std::vector<ASTNodePtr> next;
    if (auto decl_node = is_declarator()) {
        if (is_punctuator('=')) {
            if (auto init_node = is_initializer()) {
                next.push_back(std::move(decl_node));
                next.push_back(std::move(init_node));
                return MkNd(InitDeclarator);
            }
        }
        next.push_back(std::move(decl_node));
        return MkNd(InitDeclarator);
    }
    return nullptr;
}

ASTNodePtr Parser::is_initializer() {
    std::vector<ASTNodePtr> next;
    if (auto as_node = is_assignment_expression()) {
        next.push_back(std::move(as_node));
        return MkNd(Initializer);
    } else if (is_punctuator('{')) {
        if (auto init_list_node = is_initializer_list()) {
            if (is_punctuator('}')) {
                is_punctuator(',');
                next.push_back(std::move(init_list_node));
                return MkNd(Initializer);
            }
        }
        parser_error();
    }
    return nullptr;
}

ASTNodePtr Parser::is_pointer() {
    std::vector<ASTNodePtr> next;
    if (is_punctuator('*')) {
        auto list_node = is_type_qualifier_list();
        if (list_node) next.push_back(std::move(list_node));
        if (auto pointer_node = is_pointer()) {
            next.push_back(std::move(pointer_node));
            return MkNd(Pointer);
        }
        return MkNd(Pointer);
    }
    return nullptr;
}

ASTNodePtr Parser::is_parameter_type_list() {
    std::vector<ASTNodePtr> next;
    if (auto par_node = is_parameter_list()) {
        next.push_back(std::move(par_node));
        if (is_punctuator(',')) {
            if (is_keyword(TokenType::Ellipsis)) {
                auto ellipsis = MkNd(Ellipsis);
                next.push_back(std::move(ellipsis));
                return MkNd(ParameterTypeList);
            }
            parser_error();
        } else {
            return MkNd(ParameterTypeList);
        }
    }
    return nullptr;
}

ASTNodePtr Parser::is_parameter_declaration() {
    std::vector<ASTNodePtr> next;
    if (auto decls_node = is_declaration_specifiers()) {
        if (auto decl_node = is_declarator()) {
            next.push_back(std::move(decls_node));
            next.push_back(std::move(decl_node));
            return MkNd(ParameterDeclaration);
        } else {

        }
    }
    return nullptr;
}

ASTNodePtr Parser::is_specifier_qualifier_list() {
    std::vector<ASTNodePtr> next;
    auto spec_list_node = MkNd(SpecifierQualifierList);
    if (auto typeq_node = is_type_qualifier()) {
        spec_list_node->Next.push_back(std::move(typeq_node));
        if (auto spec_list2_node = is_specifier_qualifier_list())
            spec_list_node->Next.push_back(std::move(spec_list2_node));
        return spec_list_node;
    } else if (auto types_node = is_type_specifier()) {
        spec_list_node->Next.push_back(std::move(types_node));
        if (auto spec_node2 = is_specifier_qualifier_list())
            spec_list_node->Next.push_back(std::move(spec_node2));
        return spec_list_node;
    }
    return nullptr;
}

ASTNodePtr Parser::is_assignment_operator() {
    std::vector<ASTNodePtr> next;
    bool ret = MATCH_ANY(
        TokenType::MulAssign,
        TokenType::DivAssign,
        TokenType::ModAssign,
        TokenType::AddAssign,
        TokenType::SubAssign,
        TokenType::LeftAssign,
        TokenType::RightAssign,
        TokenType::AndAssign,
        TokenType::XorAssign,
        TokenType::OrAssign
    );
    if (ret) {
        advance_if(ret);
    } else if (!is_punctuator('=')) {
        return nullptr;
    }
    return MkNd(AssignmentOperator);
}

ASTNodePtr Parser::is_unary_operator() {
    std::vector<ASTNodePtr> next;
    bool ret =
        is_punctuator('&') ||
        is_punctuator('*') ||
        is_punctuator('+') ||
        is_punctuator('-') ||
        is_punctuator('~') ||
        is_punctuator('!');
    if (ret) {
        return MkNd(UnaryOperator);
    }
    return nullptr;
}

ASTNodePtr Parser::is_type_name() {
    std::vector<ASTNodePtr> next;
    if (auto spec_node = is_specifier_qualifier_list()) {
        next.push_back(std::move(spec_node));
        if (auto abs_node = is_abstract_declarator()) next.push_back(std::move(abs_node));
        return MkNd(TypeName);
    }
    return nullptr;
}

ASTNodePtr Parser::is_abstract_declarator() {
    return nullptr;
}

std::string Parser::GetUML() {
    uml_value_count_.clear();
    const auto& start_node = GetStartNode();
    uml_ss_.clear();
    uml_ss_ << "@startuml\n";
    uml_ss_ << "class translation_unit as \"translation-unit\"\n";
    for (const auto& node : start_node->Next) {
        auto name = snake_case(deserialize(node->Type));
        if (node->GetUniqueName().empty())
            node->SetUniqueName(get_unique_name(name));
        uml_ss_ << "class " << node->GetUniqueName() << " as \"" << std::regex_replace(name, std::regex("_"), "-") << "\"\n";
        uml_ss_ << "translation_unit --> " << node->GetUniqueName() << '\n';
    }
    uml_impl(start_node->Next);
    uml_ss_ << "hide members\nhide circle\n";
    uml_ss_ << "@enduml\n";
    return uml_ss_.str();
}

void Parser::uml_impl(const std::vector<ASTNodePtr>& nodes) {
    for (const auto& node : nodes) {
        uml_impl(node->Next);
        auto name = snake_case(deserialize(node->Type));
        if (node->GetUniqueName().empty())
            node->SetUniqueName(get_unique_name(name));
        uml_ss_ << "class " << node->GetUniqueName() << " as \"" << std::regex_replace(name, std::regex("_"), "-") << "\"\n";
        for (const auto& next : node->Next) {
            if (next->GetUniqueName().empty())
                next->SetUniqueName(get_unique_name(deserialize(next->Type)));
            uml_ss_ << node->GetUniqueName() << " --> " << next->GetUniqueName() << '\n';
        }
    }
}

std::string Parser::get_unique_name(std::string name) {
    std::string unique_name = name;
    if (uml_value_count_.find(name) != uml_value_count_.end()) {
        uml_value_count_[name]++;
        unique_name += std::to_string(uml_value_count_[name]);
    } else {
        uml_value_count_[name] = 0;
    }
    return unique_name;
}

TokenType Parser::get_token_type() {
    return std::get<0>(*index_);
}

std::string Parser::get_token_value() {
    return std::get<1>(*index_);
}

bool Parser::advance_if(bool adv) {
    index_ += adv;
    return adv;
}

void Parser::consume(char c) {
    if (!is_punctuator(c)) {
        std::cout << "Expected " << c << " but got " << get_token_value() << std::endl;
        parser_error();
    }
}

void Parser::consume(TokenType t) {
    if (!advance_if(get_token_type() == t)) {
        std::cout << "Expected " << deserialize(t) << " but got " << get_token_value() << std::endl;
        parser_error();
    }
}

bool Parser::type_defined(const std::string& str) {
    return typedefs_.find(str) != typedefs_.end();
}
