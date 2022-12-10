#include <lexer/lexer.hxx>
#include <parser/parser.hxx>
#include <preprocessor/preprocessor.hxx>
#include <common/log.hxx>
#include <regex>

Parser::Parser(const std::string& input)
    : input_(input)
    , tokens_{}
    , index_(nullptr)
    , start_node_{}
{
}

Parser::Parser(const std::vector<Token>& input)
    : tokens_(input)
    , input_{}
    , index_(nullptr)
    , start_node_{}
{
    index_ = tokens_.begin();
}

Parser::~Parser() {}

void Parser::Parse() {
    if (tokens_.empty()) {
        std::string unprocessed = input_;
        Preprocessor preprocessor(unprocessed);
        auto processed = preprocessor.Process();
        Lexer lexer(processed);
        tokens_ = lexer.Lex();
        if (tokens_.empty())
            parser_error;
    }
    rollback_index_ = tokens_.begin();
    rollback();
    parse_impl();
    ++index_;
}

const ASTNodePtr& Parser::GetStartNode() {
    if (index_ != tokens_.end()) {
        ERROR("Parse failed before GetStartNode");
        parser_error;
    }
    return start_node_;
}

void Parser::parse_impl() {
    if (!(start_node_ = is_translation_unit()))
        parser_error;
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
    std::vector<ASTNodePtr> next;
    // commit();
    // if (auto func_node = is_function_definition()) {
    //     next.push_back(std::move(func_node));
    // } else rollback();
    if (auto decl_node = is_declaration()) {
        next.push_back(std::move(decl_node));
    } else return nullptr;
    return MkNd(ExternalDeclaration);
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
    }
    return nullptr;
}

ASTNodePtr Parser::is_block_item() {
    std::vector<ASTNodePtr> next;
    if (auto decl_node = is_declaration()) {
        next.push_back(std::move(decl_node));
    } else if (auto stat_node = is_statement()) {
        next.push_back(std::move(stat_node));
    } else return nullptr;
    return MkNd(BlockItem);
}

ASTNodePtr Parser::is_type_specifier() {
    std::vector<ASTNodePtr> next;
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
    RETURN_IF_MATCH(Identifier, TokenType::Identifier);
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
        parser_error;
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
        parser_error;
    }
    return nullptr;
}

ASTNodePtr Parser::is_struct_declaration() {
    std::vector<ASTNodePtr> next;
    if (auto spec_node = is_specifier_qualifier_list()) {
        if (auto decl_list_node = is_struct_declarator_list()) {
            if (is_punctuator(';')) {
                next.push_back(std::move(spec_node));
                next.push_back(std::move(decl_list_node));
                return MkNd(StructDeclaration);
            }
        }
        parser_error;
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
    std::vector<ASTNodePtr> next;
    if (auto expr_node = is_conditional_expression()) {
        next.push_back(std::move(expr_node));
        return MkNd(ConstantExpression);
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
        parser_error;
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
        parser_error;
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
        parser_error;
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
        parser_error;
    }
    return MkNd(DeclarationList);
}

ASTNodePtr Parser::is_block_item_list() {
    std::vector<ASTNodePtr> next;
    if (auto item_node = is_block_item()) {
        if (auto list_node = _is_block_item_list()) {
            next.push_back(std::move(item_node));
            next.push_back(std::move(list_node));
            return MkNd(BlockItemList);
        }
        parser_error;
    }
    return nullptr;
}

ASTNodePtr Parser::_is_block_item_list() {
    std::vector<ASTNodePtr> next;
    if (auto item_node = is_block_item()) {
        if (auto list_node = _is_block_item_list()) {
            next.push_back(std::move(item_node));
            next.push_back(std::move(list_node));
            return MkNd(BlockItemList);
        }
        parser_error;
    }
    return MkNd(BlockItemList);
}

ASTNodePtr Parser::is_struct_declaration_list() {
    std::vector<ASTNodePtr> next;
    if (auto decl_node = is_struct_declaration()) {
        if (auto list_node = _is_struct_declaration_list()) {
            next.push_back(std::move(decl_node));
            next.push_back(std::move(list_node));
            return MkNd(StructDeclarationList);
        }
        parser_error;
    }
    return nullptr;
}

ASTNodePtr Parser::_is_struct_declaration_list() {
    std::vector<ASTNodePtr> next;
    if (auto decl_node = is_struct_declaration()) {
        if (auto list_node = _is_struct_declaration_list()) {
            next.push_back(std::move(decl_node));
            next.push_back(std::move(list_node));
            return MkNd(StructDeclarationList);
        }
        parser_error;
    }
    return MkNd(StructDeclarationList);
}

ASTNodePtr Parser::is_struct_declarator_list() {
    std::vector<ASTNodePtr> next;
    if (auto decl_node = is_struct_declarator()) {
        if (auto list_node = _is_struct_declarator_list()) {
            next.push_back(std::move(decl_node));
            next.push_back(std::move(list_node));
            return MkNd(StructDeclaratorList);
        }
        parser_error;
    }
    return nullptr;
}

ASTNodePtr Parser::_is_struct_declarator_list() {
    std::vector<ASTNodePtr> next;
    if (is_punctuator(',')) {
        if (auto decl_node = is_struct_declarator()) {
            if (auto list_node = _is_struct_declarator_list()) {
                next.push_back(std::move(decl_node));
                next.push_back(std::move(list_node));
                return MkNd(StructDeclaratorList);
            }
        }
        parser_error;
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
        parser_error;
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
        parser_error;
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
        parser_error;
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
        }
        parser_error;
    }
    return MkNd(ParameterList);
}

ASTNodePtr Parser::is_identifier_list() {
    std::vector<ASTNodePtr> next;
    if (is_identifier()) {
        if (auto list_node = _is_identifier_list()) {
            next.push_back(std::move(list_node));
            return MkNd(IdentifierList);
        }
        parser_error;
    }
    return nullptr;
}

ASTNodePtr Parser::_is_identifier_list() {
    std::vector<ASTNodePtr> next;
    if (is_punctuator(',')) {
        if (is_identifier()) {
            if (auto list_node = _is_identifier_list()) {
                next.push_back(std::move(list_node));
                return MkNd(IdentifierList);
            }
        }
        parser_error;
    }
    return MkNd(IdentifierList);
}

ASTNodePtr Parser::is_shift_expression() {
    std::vector<ASTNodePtr> next;
    if (auto expr_node = is_additive_expression()) {
        if (auto expr_node2 = _is_shift_expression()) {
            next.push_back(std::move(expr_node));
            next.push_back(std::move(expr_node2));
            return MkNd(ShiftExpression);
        }
        parser_error;
    }
    return nullptr;
}

ASTNodePtr Parser::_is_shift_expression() {
    std::vector<ASTNodePtr> next;
    if (is_keyword(TokenType::LeftOp) || is_keyword(TokenType::RightOp)) {
        if (auto expr_node = is_additive_expression()) {
            if (auto expr_node2 = _is_shift_expression()) {
                next.push_back(std::move(expr_node));
                next.push_back(std::move(expr_node2));
                return MkNd(ShiftExpression);
            }
        }
        parser_error;
    }
    return MkNd(ShiftExpression);
}

ASTNodePtr Parser::is_additive_expression() {
    std::vector<ASTNodePtr> next;
    if (auto expr_node = is_multiplicative_expression()) {
        if (auto expr_node2 = _is_additive_expression()) {
            next.push_back(std::move(expr_node));
            next.push_back(std::move(expr_node2));
            return MkNd(AdditiveExpression);
        }
        parser_error;
    }
    return nullptr;
}

ASTNodePtr Parser::_is_additive_expression() {
    std::vector<ASTNodePtr> next;
    if (is_punctuator('+') || is_punctuator('-')) {
        if (auto expr_node = is_multiplicative_expression()) {
            if (auto expr_node2 = _is_additive_expression()) {
                next.push_back(std::move(expr_node));
                next.push_back(std::move(expr_node2));
                return MkNd(AdditiveExpression);
            }
        }
        parser_error;
    }
    return MkNd(AdditiveExpression);
}

ASTNodePtr Parser::is_multiplicative_expression() {
    std::vector<ASTNodePtr> next;
    if (auto expr_node = is_cast_expression()) {
        if (auto expr_node2 = _is_multiplicative_expression()) {
            next.push_back(std::move(expr_node));
            next.push_back(std::move(expr_node2));
            return MkNd(MultiplicativeExpression);
        }
        parser_error;
    }
    return nullptr;
}

ASTNodePtr Parser::_is_multiplicative_expression() {
    std::vector<ASTNodePtr> next;
    if (is_punctuator('*') || is_punctuator('/') || is_punctuator('%')) {
        if (auto expr_node = is_cast_expression()) {
            if (auto expr_node2 = _is_multiplicative_expression()) {
                next.push_back(std::move(expr_node));
                next.push_back(std::move(expr_node2));
                return MkNd(MultiplicativeExpression);
            }
        }
        parser_error;
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
        parser_error;
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
        parser_error;
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
        parser_error;
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
        parser_error;
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
        parser_error;
    } else if (auto init_node = is_initializer()) {
        if (auto list_node = _is_initializer_list()) {
            next.push_back(std::move(init_node));
            next.push_back(std::move(list_node));
            return MkNd(InitializerList);
        }
        parser_error;
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
        parser_error;
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
        parser_error;
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
        parser_error;
    }
    return MkNd(DesignatorList);
}

ASTNodePtr Parser::is_designator() {
    std::vector<ASTNodePtr> next;
    if (is_punctuator('[')) {
        if (auto expr_node = is_constant_expression()) {
            if (is_punctuator(']')) {
                next.push_back(std::move(expr_node));
                return MkNd(Designator);
            }
        }
        parser_error;
    } else if (is_punctuator('.')) {
        if (auto id_node = is_identifier()) {
            next.push_back(std::move(id_node));
            return MkNd(Designator);
        }
        parser_error;
    }
    return nullptr;
}

ASTNodePtr Parser::is_designation() {
    std::vector<ASTNodePtr> next;
    if (auto desi_node = is_designator_list()) {
        if (is_punctuator('=')) {
            next.push_back(std::move(desi_node));
            return MkNd(Designation);
        }
        parser_error;
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
        parser_error;
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
                parser_error;
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
        if (is_punctuator(';')) {
            if (init_node) next.push_back(std::move(init_node));
            next.push_back(std::move(decl_node));
            return MkNd(Declaration);
        }
    }
    return nullptr;
}

ASTNodePtr Parser::is_primary_expression() {
    std::vector<ASTNodePtr> next;
    if (auto id_node = is_identifier()) {
        next.push_back(std::move(id_node));
        return MkNd(PrimaryExpression);
    } else if (auto num_node = is_constant()) {
        next.push_back(std::move(num_node));
        return MkNd(PrimaryExpression);
    } else if (auto str_node = is_string_literal()) {
        next.push_back(std::move(str_node));
        return MkNd(PrimaryExpression);
    } else if (is_punctuator('(')) {
        if (auto expr_node = is_expression()) {
            if (is_punctuator(')')) {
                next.push_back(std::move(expr_node));
                return MkNd(PrimaryExpression);
            }
        }
        parser_error;
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
    std::vector<ASTNodePtr> next;
    if (auto l_stat_node = is_labeled_statement()) {
        next.push_back(std::move(l_stat_node));
    } else if (auto c_stat_node = is_compound_statement()) {
        next.push_back(std::move(c_stat_node));
    } else if (auto e_stat_node = is_expression_statement()) {
        next.push_back(std::move(e_stat_node));
    } else if (auto s_stat_node = is_selection_statement()) {
        next.push_back(std::move(s_stat_node));
    } else if (auto i_stat_node = is_iteration_statement()) {
        next.push_back(std::move(i_stat_node));
    } else if (auto j_stat_node = is_jump_statement()) {
        next.push_back(std::move(j_stat_node));
    } else return nullptr;
    return MkNd(Statement);
}

ASTNodePtr Parser::is_selection_statement() {
    std::vector<ASTNodePtr> next;
    if (is_keyword(TokenType::If)) {
        if (is_punctuator('(')) {
            if (auto expr_node = is_expression()) {
                if (is_punctuator(')')) {
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
            }
        }
        parser_error;
    } else if (is_keyword(TokenType::Switch)) {
        if (is_punctuator('(')) {
            if (auto expr_node = is_expression()) {
                if (is_punctuator(')')) {
                    if (auto stat_node = is_statement()) {
                        next.push_back(std::move(expr_node));
                        next.push_back(std::move(stat_node));
                        return MkNd(SelectionStatement);
                    }
                }
            }
        }
        parser_error;
    }
    return nullptr;
}

ASTNodePtr Parser::is_iteration_statement() {
    std::vector<ASTNodePtr> next;
    if (is_keyword(TokenType::While)) {
        if (is_punctuator('(')) {
            if (auto expr_node = is_expression()) {
                if (is_punctuator(')')) {
                    if (auto stat_node = is_statement()) {
                        next.push_back(std::move(expr_node));
                        next.push_back(std::move(stat_node));
                        return MkNd(IterationStatement);
                    }
                }
            }
        }
        parser_error;
    } else if (is_keyword(TokenType::Do)) {
        if (auto stat_node = is_statement()) {
            if (is_punctuator('(')) {
                if (auto expr_node = is_expression()) {
                    if (is_punctuator(')')) {
                        next.push_back(std::move(expr_node));
                        next.push_back(std::move(stat_node));
                        return MkNd(IterationStatement);
                    }
                }
            }
        }
        parser_error;
    } else if (is_keyword(TokenType::For)) {
        if (is_punctuator('(')) {
            auto expr_node = is_expression();
            if (is_punctuator(';')) {
                if (is_punctuator(';')) {
                    auto expr2_node = is_expression();
                    if (is_punctuator(';')) {
                        auto expr3_node = is_expression();
                        if (is_punctuator(')')) {
                            if (auto stat_node = is_statement()) {
                                if (expr_node) next.push_back(std::move(expr_node));
                                if (expr2_node) next.push_back(std::move(expr2_node));
                                if (expr3_node) next.push_back(std::move(expr3_node));
                                next.push_back(std::move(stat_node));
                                return MkNd(IterationStatement);
                            }
                        }
                    }
                }
            } else if (auto decl_node = is_declaration()) {
                auto expr2_node = is_expression();
                if (is_punctuator(';')) {
                    auto expr3_node = is_expression();
                    if (is_punctuator(')')) {
                        if (auto stat_node = is_statement()) {
                            if (expr_node) parser_error;
                            if (expr2_node) next.push_back(std::move(expr2_node));
                            if (expr3_node) next.push_back(std::move(expr3_node));
                            next.push_back(std::move(stat_node));
                            return MkNd(IterationStatement);
                        }
                    }
                }
            }
        }
        parser_error;
    }
    return nullptr;
}

ASTNodePtr Parser::is_jump_statement() {
    std::vector<ASTNodePtr> next;
    if (is_keyword(TokenType::Goto)) {
        if (auto id = is_identifier()) {
            if (is_punctuator(';')) {
                next.push_back(std::move(id));
                return MkNd(JumpStatement);
            }
        }
        parser_error;
    } else if (is_keyword(TokenType::Continue)) {
        if (is_punctuator(';')) {
            return MkNd(JumpStatement);
        }
        parser_error;
    } else if (is_keyword(TokenType::Break)) {
        if (is_punctuator(';')) {
            return MkNd(JumpStatement);
        }
        parser_error;
    } else if (is_keyword(TokenType::Return)) {
        auto expr_node = is_expression();
        if (is_punctuator(';')) {
            if (expr_node) next.push_back(std::move(expr_node));
            return MkNd(JumpStatement);
        }
        parser_error;
    }
    return nullptr;
}

ASTNodePtr Parser::is_expression_statement() {
    auto expr_node = is_expression();
    if (is_punctuator(';')) {
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
        }
        parser_error;
    } else if (is_keyword(TokenType::Case)) {
        if (auto const_node = is_constant_expression()) {
            if (is_punctuator(':')) {
                if (auto stat_node = is_statement()) {
                    next.push_back(std::move(const_node));
                    next.push_back(std::move(stat_node));
                    return MkNd(LabeledStatement);
                }
            }
        }
        parser_error;
    } else if (is_keyword(TokenType::Default)) {
        if (is_punctuator(':')) {
            if (auto stat_node = is_statement()) {
                next.push_back(std::move(stat_node));
                return MkNd(LabeledStatement);
            }
        }
        parser_error;
    }
    return nullptr;
}

ASTNodePtr Parser::is_compound_statement() {
    if (is_punctuator('{')) {
        std::vector<ASTNodePtr> next;
        auto block_list = is_block_item_list();
        if (is_punctuator('}')) {
            if (block_list) next.push_back(std::move(block_list));
            return MkNd(CompoundStatement);
        }
        parser_error;
    }
    return nullptr;
}

ASTNodePtr Parser::is_expression() {
    std::vector<ASTNodePtr> next;
    if (auto assi_node = is_assignment_expression()) {
        next.push_back(std::move(assi_node));
        return MkNd(Expression);
    } else if (auto expr_node = is_expression()) {
        if (is_punctuator(',')) {
            if (auto assi_node2 = is_assignment_expression()) {
                next.push_back(std::move(expr_node));
                next.push_back(std::move(assi_node2));
                return MkNd(Expression);
            }
        }
        parser_error;
    }
    return nullptr;
}

ASTNodePtr Parser::is_assignment_expression() {
    std::vector<ASTNodePtr> next;
    if (auto cond_node = is_conditional_expression()) {
        next.push_back(std::move(cond_node));
        return MkNd(AssignmentExpression);
    } else if (auto un_node = is_unary_expression()) {
        if (auto assi_oper_node = is_assignment_operator()) {
            if (auto assi_node = is_assignment_expression()) {
                next.push_back(std::move(un_node));
                next.push_back(std::move(assi_oper_node));
                next.push_back(std::move(assi_node));
                return MkNd(AssignmentExpression);
            }
        }
        parser_error;
    }
    return nullptr;
}

ASTNodePtr Parser::is_conditional_expression() {
    std::vector<ASTNodePtr> next;
    if (auto or_node = is_logical_or_expression()) {
        next.push_back(std::move(or_node));
        if (is_punctuator('?')) {
            if (auto expr_node = is_expression()) {
                if (is_punctuator(':')) {
                    if (auto cond_node = is_conditional_expression()) {
                        next.push_back(std::move(expr_node));
                        next.push_back(std::move(cond_node));
                        return MkNd(ConditionalExpression);
                    }
                }
            }
            parser_error;
        }
        return MkNd(ConditionalExpression);
    }
    return nullptr;
}

ASTNodePtr Parser::is_logical_or_expression() {
    std::vector<ASTNodePtr> next;
    if (auto and_node = is_logical_and_expression()) {
        next.push_back(std::move(and_node));
        return MkNd(LogicalOrExpression);
    } else if (auto or_node = is_logical_or_expression()) {
        if (is_keyword(TokenType::OrOp)) {
            if (auto and_node = is_logical_and_expression()) {
                next.push_back(std::move(or_node));
                next.push_back(std::move(and_node));
                return MkNd(LogicalOrExpression);
            }
        }
        parser_error;
    }
    return nullptr;
}

ASTNodePtr Parser::is_logical_and_expression() {
    std::vector<ASTNodePtr> next;
    if (auto or_node = is_inclusive_or_expression()) {
        next.push_back(std::move(or_node));
        return MkNd(LogicalAndExpression);
    } else if (auto and_node = is_logical_and_expression()) {
        if (is_keyword(TokenType::AndOp)) {
            if (auto or_node = is_inclusive_or_expression()) {
                next.push_back(std::move(and_node));
                next.push_back(std::move(or_node));
                return MkNd(LogicalAndExpression);
            }
        }
        parser_error;
    }
    return nullptr;
}

ASTNodePtr Parser::is_inclusive_or_expression() {
    std::vector<ASTNodePtr> next;
    if (auto ex_node = is_exclusive_or_expression()) {
        next.push_back(std::move(ex_node));
        return MkNd(InclusiveOrExpression);
    } else if (auto or_node = is_inclusive_or_expression()) {
        if (is_punctuator('|')) {
            if (auto ex_node = is_exclusive_or_expression()) {
                next.push_back(std::move(or_node));
                next.push_back(std::move(ex_node));
                return MkNd(InclusiveOrExpression);
            }
        }
        parser_error;
    }
    return nullptr;
}

ASTNodePtr Parser::is_exclusive_or_expression() {
    std::vector<ASTNodePtr> next;
    if (auto and_node = is_and_expression()) {
        next.push_back(std::move(and_node));
        return MkNd(ExclusiveOrExpression);
    } else if (auto ex_node = is_exclusive_or_expression()) {
        if (is_punctuator('^')) {
            if (auto and_node = is_and_expression()) {
                next.push_back(std::move(ex_node));
                next.push_back(std::move(and_node));
                return MkNd(ExclusiveOrExpression);
            }
        }
        parser_error;
    }
    return nullptr;
}

ASTNodePtr Parser::is_and_expression() {
    std::vector<ASTNodePtr> next;
    if (auto eq_node = is_equality_expression()) {
        next.push_back(std::move(eq_node));
        return MkNd(AndExpression);
    } else if (auto and_node = is_and_expression()) {
        if (is_punctuator('&')) {
            if (auto eq_node = is_equality_expression()) {
                next.push_back(std::move(and_node));
                next.push_back(std::move(eq_node));
                return MkNd(AndExpression);
            }
        }
    }
    return nullptr;
}

ASTNodePtr Parser::is_equality_expression() {
    std::vector<ASTNodePtr> next;
    if (auto ret_node = is_relational_expression()) {
        next.push_back(std::move(ret_node));
        return MkNd(EqualityExpression);
    } else if (auto eq_node = is_equality_expression()) {
        if (is_keyword(TokenType::EqOp) || is_keyword(TokenType::NeOp)) {
            if (auto ret_node = is_relational_expression()) {
                next.push_back(std::move(eq_node));
                next.push_back(std::move(ret_node));
                return MkNd(EqualityExpression);
            }
        }
        parser_error;
    }
    return nullptr;
}

ASTNodePtr Parser::is_relational_expression() {
    std::vector<ASTNodePtr> next;
    if (auto sh_node = is_shift_expression()) {
        next.push_back(std::move(sh_node));
        return MkNd(RelationalExpression);
    } else if (auto req_node = is_relational_expression()) {
        if (is_keyword(TokenType::GeOp) || is_keyword(TokenType::LeOp)
            || is_punctuator('<') || is_punctuator('>'))
        {
            if (auto sh_node = is_shift_expression()) {
                next.push_back(std::move(req_node));
                next.push_back(std::move(sh_node));
                return MkNd(RelationalExpression);
            }
        }
        parser_error;
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
    // TODO: careful, this swallows identifiers, need a list of typedef'd names before
    // this is implemented
    std::vector<ASTNodePtr> next;
    if (auto id_node = is_identifier()) {
        next.push_back(std::move(id_node));
        return MkNd(TypedefName);
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
        parser_error;
    } else if (auto dir_node = is_direct_declarator()) {
        next.push_back(std::move(dir_node));
        return MkNd(Declarator);
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
        parser_error;
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

ASTNodePtr Parser::is_direct_declarator() {
    std::vector<ASTNodePtr> next;
    if (auto id = is_identifier()) {
        next.push_back(std::move(id));
        if (is_punctuator('[')) {

        } else if (is_punctuator('(')) {
            if (auto par_node = is_parameter_type_list()) {
                if (is_punctuator(')')) {
                    next.push_back(std::move(par_node));
                    return MkNd(DirectDeclarator);
                }
            } else {
                auto id_list_node = is_identifier_list();
                if (is_punctuator(')')) {
                    if (id_list_node) next.push_back(std::move(id_list_node));
                    return MkNd(DirectDeclarator);
                }
            }
        }
        return MkNd(DirectDeclarator);
    } else if (is_punctuator('(')) {
        if (auto decl_node = is_declarator()) {
            if (is_punctuator(')')) {
                next.push_back(std::move(decl_node));
                if (is_punctuator('[')) {

                } else if (is_punctuator('(')) {
                    if (auto par_node = is_parameter_type_list()) {
                        if (is_punctuator(')')) {
                            next.push_back(std::move(par_node));
                            return MkNd(DirectDeclarator);
                        }
                    } else {
                        auto id_list_node = is_identifier_list();
                        if (is_punctuator(')')) {
                            if (id_list_node) next.push_back(std::move(id_list_node));
                            return MkNd(DirectDeclarator);
                        }
                    }
                }
                return MkNd(DirectDeclarator);
            }
        }
        parser_error;
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
            parser_error;
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

ASTNodePtr Parser::is_cast_expression() {
    std::vector<ASTNodePtr> next;
    if (auto un_node = is_unary_expression()) {
        next.push_back(std::move(un_node));
        return MkNd(CastExpression);
    } else if (is_punctuator('(')) {
        if (auto type_node = is_type_name()) {
            if (is_punctuator(')')) {
                if (auto cast_node = is_cast_expression()) {
                    next.push_back(std::move(type_node));
                    next.push_back(std::move(cast_node));
                    return MkNd(CastExpression);
                }
            }
        }
    }
    return nullptr;
}

ASTNodePtr Parser::is_unary_expression() {
    std::vector<ASTNodePtr> next;
    if (auto id = is_identifier()) {
        next.push_back(std::move(id));
        return MkNd(UnaryExpression);
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
        auto name = deserialize(node->Type);
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
        auto name = deserialize(node->Type);
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

void Parser::commit() {
    rollback_index_ = index_;
}

void Parser::rollback() {
    index_ = rollback_index_;
}