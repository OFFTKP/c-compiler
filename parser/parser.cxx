#include <lexer/lexer.hxx>
#include <parser/parser.hxx>
#include <parser/parser_defines.hxx>
#include <preprocessor/preprocessor.hxx>
#include <common/log.hxx>
#include <regex>

Parser::Parser(const std::string& input)
    : input_(input)
    , index_(nullptr)
    , start_node_{}
{
}

Parser::~Parser() {}

void Parser::Parse() {
    std::string unprocessed = input_;
    Preprocessor preprocessor(unprocessed);
    auto processed = preprocessor.Process();
    Lexer lexer(processed);
    tokens_ = lexer.Lex();
    if (tokens_.empty())
        throw_error();
    rollback_index_ = tokens_.begin();
    
    rollback();
    parse_impl();
    ++index_;
}

const ASTNodePtr& Parser::GetStartNode() {
    if (index_ != tokens_.end()) {
        ERROR("Parse failed before GetStartNode");
        throw_error();
    }
    return start_node_;
}

void Parser::parse_impl() {
    if (!(start_node_ = is_translation_unit()))
        throw_error();
}

ASTNodePtr Parser::is_translation_unit() {
    start_node_ = MakeNode(ASTNodeType::Start, {}, "Start");
    std::vector<ASTNodePtr> next;
    while(!MATCH_ANY(TokenType::Eof)) {
        if (auto ext = is_external_declaration()) {
            next.push_back(std::move(ext));
        }
    }
    start_node_->Next = std::move(next);
    return std::move(start_node_);
}

ASTNodePtr Parser::is_external_declaration() {
    std::vector<ASTNodePtr> next;
    if (auto func_node = is_function_declaration()) {
        next.push_back(std::move(func_node));
    } else if (auto decl_node = is_declaration()) {
        next.push_back(std::move(decl_node));
    } else return nullptr;
    return MakeNode(ASTNodeType::ExternalDeclaration, std::move(next), GET_UNUSED_NAME("external_declaration"));
}

ASTNodePtr Parser::is_function_declaration() {
    // if (auto type_node = is_type_specifier()) {
    //     if (auto id_node = is_identifier()) {
    //         if (auto args_node = is_function_arguments()) {
    //             if (auto block_node = is_code_block()) {
    //                 auto name = id_node->Value;
    //                 std::vector<ASTNodePtr> next;
    //                 next.push_back(std::move(type_node));
    //                 next.push_back(std::move(args_node));
    //                 next.push_back(std::move(block_node));
    //                 return MakeNode(ASTNodeType::FunctionDeclaration, std::move(next), std::move(name));
    //             }
    //         }
    //     }
    //     throw_error();
    // }
    return nullptr;
}

ASTNodePtr Parser::is_type_specifier() {
    std::vector<ASTNodePtr> next;
    auto value = get_token_value();
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
        return MakeNode(ASTNodeType::TypeSpecifier, {}, GET_UNUSED_NAME(value));
    } else if (auto struct_node = is_struct_or_union_specifier()) {
        next.push_back(std::move(struct_node));
    } else if (auto enum_node = is_enum_specifier()) {
        next.push_back(std::move(enum_node));
    } else if (auto typedef_node = is_typedef_name()) {
        next.push_back(std::move(typedef_node));
    } else return nullptr;
    return MakeNode(ASTNodeType::TypeSpecifier, std::move(next), GET_UNUSED_NAME(value));
}

ASTNodePtr Parser::is_function_specifier() {
    RETURN_IF_MATCH(FunctionSpecifier, TokenType::Inline);
}

ASTNodePtr Parser::is_identifier() {
    RETURN_IF_MATCH(Identifier, TokenType::Identifier);
}

ASTNodePtr Parser::is_function_arguments() {
    if (auto lpar_node = is_punctuator('(')) {
        if (auto arg_list_node = is_argument_list()) {
            if (auto rpar_node = is_punctuator(')')) {
                std::vector<ASTNodePtr> next;
                next.push_back(std::move(arg_list_node));
                return MakeNode(ASTNodeType::FunctionArguments, std::move(next), GET_UNUSED_NAME("function_arguments"));
            }
        }
        throw_error();
    }
    return nullptr;
}

bool Parser::is_punctuator(char c) {
    return advance_if(get_token_type() == TokenType::Punctuator && get_token_value()[0] == c);
}

ASTNodePtr Parser::is_code_block() {
    if (auto lbra_node = is_punctuator('{')) {
        if (auto stat_node = is_statement_list()) {
            if (auto rbra_node = is_punctuator('}')) {
                std::vector<ASTNodePtr> next;
                next.push_back(std::move(stat_node));
                return MakeNode(ASTNodeType::CodeBlock, std::move(next), GET_UNUSED_NAME("code_block"));
            }
        }
        throw_error();
    }
    return nullptr;
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
    if (auto str_node = is_struct_or_union()) {
        std::vector<ASTNodePtr> next;
        auto id_node = is_identifier();
        if (is_punctuator('{')) {
            if (auto decl_list_node = is_struct_declaration_list()) {
                if (is_punctuator('}')) {
                    next.push_back(std::move(str_node));
                    if (id_node) next.push_back(std::move(id_node));
                    next.push_back(std::move(decl_list_node));
                    return MakeNode(ASTNodeType::StructOrUnionSpecifier, std::move(next), GET_UNUSED_NAME("struct_or_union_specifier"));
                }
            }
        }
        throw_error();
    }
    return nullptr;
}

ASTNodePtr Parser::is_struct_declaration_list() {
    MAKE_LIST(struct_declaration_list, struct_declaration, StructDeclarationList);
}

ASTNodePtr Parser::is_struct_declaration() {
    if (auto spec_qual_list_node = is_specifier_qualifier_list()) {
        if (auto decl_list_node = is_struct_declarator_list()) {
            if (is_punctuator(';')) {
                std::vector<ASTNodePtr> next;
                next.push_back(std::move(spec_qual_list_node));
                next.push_back(std::move(decl_list_node));
                return MakeNode(ASTNodeType::StructDeclaration, std::move(next), GET_UNUSED_NAME("struct_declaration"));
            }
        }
    }
    return nullptr;
}

ASTNodePtr Parser::is_struct_declarator_list() {
    ASTNodePtr list_1 = MakeNode(ASTNodeType::StructDeclaratorList, {}, GET_UNUSED_NAME("struct_declaration_list"));
    if (auto member_node = is_struct_declarator()) {
        if (auto list_2 = is_struct_declarator_list())
            for (auto& node : list_2->Next)
                list_1->Next.push_back(std::move(node));
        list_1->Next.push_back(std::move(member_node));
        return list_1;
    } else if (is_punctuator(',')) {
        if (auto member_node = is_struct_declarator()) {
            if (auto list_2 = is_struct_declarator_list())
                for (auto& node : list_2->Next)
                    list_1->Next.push_back(std::move(node));
            list_1->Next.push_back(std::move(member_node));
            return list_1;
        }
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
    return MakeNode(ASTNodeType::StructDeclarator, std::move(next), GET_UNUSED_NAME("struct_declarator"));
}

ASTNodePtr Parser::is_constant_expression() {
    return nullptr;
}

ASTNodePtr Parser::is_argument_list() {
    if (auto arg_node = is_argument()) {
        std::vector<ASTNodePtr> next;
        next.push_back(std::move(arg_node));
        auto args_node = MakeNode(ASTNodeType::ArgumentList, std::move(next), GET_UNUSED_NAME("arguments"));
        if (auto comma_node = is_punctuator(',')) {
            if (auto args_node2 = is_argument_list()) {
                for (auto& arg : args_node2->Next) {
                    args_node->Next.push_back(std::move(arg));
                }
                return args_node;
            } else {
                throw_error();
            }
        } else {
            return args_node;
        }
    }
    return MakeNode(ASTNodeType::ArgumentList, {}, GET_UNUSED_NAME("arguments"));
}

ASTNodePtr Parser::is_argument() {
    if (auto type_node = is_type_specifier()) {
        auto id_node = is_identifier();
        std::vector<ASTNodePtr> next;
        next.push_back(std::move(type_node));
        next.push_back(std::move(id_node));
        return MakeNode(ASTNodeType::Argument, std::move(next), GET_UNUSED_NAME("argument"));
    }
    return nullptr;
}

ASTNodePtr Parser::is_declaration() {
    if (auto decl_node = is_declaration_specifiers()) {
        std::vector<ASTNodePtr> next;
        auto init_node = is_init_declarator_list();
        if (is_punctuator(';')) {
            if (init_node) next.push_back(std::move(init_node));
            next.push_back(std::move(decl_node));
            return MakeNode(ASTNodeType::Declaration, std::move(next), GET_UNUSED_NAME("declaration"));
        }
    }
    return nullptr;
}

ASTNodePtr Parser::is_declaration_specifiers() {
    // TODO unfinished
    std::vector<ASTNodePtr> next;
    if (auto type_node = is_type_specifier()) {
        auto decl_node = is_declaration_specifiers();
        if (decl_node) next.push_back(std::move(decl_node));
        next.push_back(std::move(type_node));
        return MakeNode(ASTNodeType::DeclarationSpecifiers, std::move(next), GET_UNUSED_NAME("declaration_specifiers"));
    }
    return nullptr;
}

ASTNodePtr Parser::is_statement_list() {
    return MakeNode(ASTNodeType::StatementList, {}, GET_UNUSED_NAME("statement_list"));
}

ASTNodePtr Parser::is_statement() {
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
}

ASTNodePtr Parser::is_init_declarator_list() {
    return nullptr;
}

ASTNodePtr Parser::is_declarator() {
    std::vector<ASTNodePtr> next;
    auto p_node = is_pointer();
    if (auto decl_node = is_direct_declarator()) {
        if (p_node)
            next.push_back(std::move(p_node));
        next.push_back(std::move(decl_node));
        return MakeNode(ASTNodeType::Declarator, std::move(next), GET_UNUSED_NAME("declarator"));
    }
    return nullptr;
}

ASTNodePtr Parser::is_pointer() {
    std::vector<ASTNodePtr> next;
    if (is_punctuator('*')) {
        auto list_node = is_type_qualifier_list();
        if (auto pointer_node = is_pointer()) {
            if (list_node) next.push_back(std::move(list_node));
            next.push_back(std::move(pointer_node));
            return MakeNode(ASTNodeType::Pointer, std::move(next), GET_UNUSED_NAME("pointer"));
        }
        return MakeNode(ASTNodeType::Pointer, {}, GET_UNUSED_NAME("pointer"));
    }
    return nullptr;
}

ASTNodePtr Parser::is_direct_declarator() {
    std::vector<ASTNodePtr> next;
    if (auto id = is_identifier()) {
        next.push_back(std::move(id));
        return MakeNode(ASTNodeType::Identifier, std::move(next), GET_UNUSED_NAME("direct_declarator"));
    }
    return nullptr;
}

ASTNodePtr Parser::is_type_qualifier_list() {
    MAKE_LIST(type_qualifier_list, type_qualifier, TypeQualifierList);
}

ASTNodePtr Parser::is_specifier_qualifier_list() {
    auto spec_list_node = MakeNode(ASTNodeType::SpecifierQualifierList, {}, GET_UNUSED_NAME("specifier_qualifier_list"));
    if (auto typeq_node = is_type_qualifier()) {
        if (auto spec_list2_node = is_specifier_qualifier_list())
            for (auto& node : spec_list2_node->Next)
                spec_list_node->Next.push_back(std::move(node));
        spec_list_node->Next.push_back(std::move(typeq_node));
        return spec_list_node;
    } else if (auto types_node = is_type_specifier()) {
        if (auto spec_node2 = is_specifier_qualifier_list())
            for (auto& node : spec_node2->Next)
                spec_list_node->Next.push_back(std::move(node));
        spec_list_node->Next.push_back(std::move(types_node));
        return spec_list_node;
    }
    return nullptr;
}

ASTNodePtr Parser::is_assignment_operator() {
    auto value = get_token_value();
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
    return MakeNode(ASTNodeType::AssignmentOperator, {}, std::move(value));
}

ASTNodePtr Parser::is_unary_operator() {
    auto value = get_token_value();
    bool ret =
        is_punctuator('&') ||
        is_punctuator('*') ||
        is_punctuator('+') ||
        is_punctuator('-') ||
        is_punctuator('~') ||
        is_punctuator('!');
    if (ret) {
        return MakeNode(ASTNodeType::UnaryOperator, {}, std::move(value));
    }
    return nullptr;
}

ASTNodePtr Parser::is_cast_expression() {
    auto value = get_token_value();
    if (auto un_node = is_unary_operator()) {
        std::vector<ASTNodePtr> next;
        next.push_back(std::move(un_node));
        return MakeNode(ASTNodeType::CastExpression, std::move(next), std::move(value));
    } else if (is_punctuator('(')) {
        if (auto type_node = is_type_name()) {
            if (is_punctuator(')')) {
                if (auto un_expr_node = is_unary_expression()) {
                    std::vector<ASTNodePtr> next;
                    next.push_back(std::move(type_node));
                    next.push_back(std::move(un_expr_node));
                    return MakeNode(ASTNodeType::CastExpression, std::move(next), std::move(value));
                }
            }
        }
    }
    return nullptr;
}

ASTNodePtr Parser::is_type_name() {
    return nullptr;
}

ASTNodePtr Parser::is_unary_expression() {
    return nullptr;
}

std::string Parser::GetUML() {
    const auto& start_node = GetStartNode();
    uml_ss_.clear();
    uml_ss_ << "@startuml\n";
    uml_ss_ << "class translation_unit as \"translation unit\"\n";
    for (const auto& node : start_node->Next) {
        auto value = std::regex_replace(node->Value, std::regex("\\d+?__"), "");
        uml_ss_ << "class " << node->Value << " as \"" << value << "\"\n";
        uml_ss_ << "translation_unit --> " << node->Value << '\n';
    }
    uml_impl(start_node->Next);
    uml_ss_ << "hide members\nhide circle\n";
    uml_ss_ << "@enduml\n";
    return uml_ss_.str();
}

void Parser::uml_impl(const std::vector<ASTNodePtr>& nodes) {
    for (const auto& node : nodes) {
        uml_impl(node->Next);
        auto value = get_uml_name(node->Value);
        uml_ss_ << "class " << node->Value << " as \"" << value << "\"\n";
        for (const auto& next : node->Next) {
            uml_ss_ << node->Value << " --> " << next->Value << '\n';
        }
    }
}

TokenType Parser::get_token_type() {
    return std::get<0>(*index_);
}

std::string Parser::get_token_value() {
    return std::get<1>(*index_);
}

std::string Parser::get_uml_name(const std::string& name) {
    std::string ret;
    ret = std::regex_replace(name, std::regex("\\d+?__"), "");
    return ret;
}

bool Parser::advance_if(bool adv) {
    index_ += adv;
    return adv;
}

void Parser::throw_error() {
    throw std::runtime_error("Parse error");
}

void Parser::commit() {
    rollback_index_ = index_;
}

void Parser::rollback() {
    index_ = rollback_index_;
}