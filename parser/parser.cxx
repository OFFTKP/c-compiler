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
            throw_error();
    }
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
    commit();
    if (auto func_node = is_function_definition()) {
        next.push_back(std::move(func_node));
    } else rollback();
    if (auto decl_node = is_declaration()) {
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

ASTNodePtr Parser::is_function_definition() {
    if (auto decl_spec_node = is_declaration_specifiers()) {
        std::vector<ASTNodePtr> next;
        if (auto decl_node = is_declarator()) {
            auto decl_list_node = is_declaration_list();
            if (auto comp_node = is_compound_statement()) {
                next.push_back(std::move(decl_spec_node));
                next.push_back(std::move(decl_node));
                if (decl_list_node) next.push_back(std::move(decl_list_node));
                next.push_back(std::move(comp_node));
                return MakeNode(ASTNodeType::FunctionDefinition, std::move(next), GET_UNUSED_NAME("function_definition"));
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
    return MakeNode(ASTNodeType::BlockItem, std::move(next), GET_UNUSED_NAME("block_item"));
}

ASTNodePtr Parser::is_type_specifier() {
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
        return MakeNode(ASTNodeType::TypeSpecifier, {}, GET_UNUSED_NAME("type_specifier"));
    } else if (auto struct_node = is_struct_or_union_specifier()) {
        return std::move(struct_node);
    } else if (auto enum_node = is_enum_specifier()) {
        return std::move(enum_node);
    } else if (auto typedef_node = is_typedef_name()) {
        return std::move(typedef_node);
    } else return nullptr;
}

ASTNodePtr Parser::is_function_specifier() {
    RETURN_IF_MATCH(FunctionSpecifier, "function_specifier", TokenType::Inline);
}

ASTNodePtr Parser::is_identifier() {
    RETURN_IF_MATCH(Identifier, "identifier", TokenType::Identifier);
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

bool Parser::is_keyword(TokenType t) {
    return advance_if(get_token_type() == t);
}

ASTNodePtr Parser::is_storage_class_specifier() {
    RETURN_IF_MATCH(StorageClassSpecifier, "storage_class_specifier",
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
    if  (auto storage_node = is_storage_class_specifier()) {
        auto decl_node = is_declaration_specifiers();
        if (decl_node) next.push_back(std::move(decl_node));
        next.push_back(std::move(storage_node));
        return MakeNode(ASTNodeType::DeclarationSpecifiers, std::move(next), GET_UNUSED_NAME("declaration_specifiers"));
    } else if (auto types_node = is_type_specifier()) {
        auto decl_node = is_declaration_specifiers();
        if (decl_node) next.push_back(std::move(decl_node));
        next.push_back(std::move(types_node));
        return MakeNode(ASTNodeType::DeclarationSpecifiers, std::move(next), GET_UNUSED_NAME("declaration_specifiers"));
    } else if (auto typeq_node = is_type_qualifier()) {
        auto decl_node = is_declaration_specifiers();
        if (decl_node) next.push_back(std::move(decl_node));
        next.push_back(std::move(typeq_node));
        return MakeNode(ASTNodeType::DeclarationSpecifiers, std::move(next), GET_UNUSED_NAME("declaration_specifiers"));
    } else if (auto spec_node = is_function_specifier()) {
        auto decl_node = is_declaration_specifiers();
        if (decl_node) next.push_back(std::move(decl_node));
        next.push_back(std::move(spec_node));
        return MakeNode(ASTNodeType::DeclarationSpecifiers, std::move(next), GET_UNUSED_NAME("declaration_specifiers"));
    }
    return nullptr;
}

ASTNodePtr Parser::is_statement_list() {
    return MakeNode(ASTNodeType::StatementList, {}, GET_UNUSED_NAME("statement_list"));
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
    return MakeNode(ASTNodeType::Statement, std::move(next), GET_UNUSED_NAME("statement"));
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
                                return MakeNode(ASTNodeType(ASTNodeType::SelectionStatement), std::move(next), GET_UNUSED_NAME("selection_statement"));
                            }
                        } else {
                            return MakeNode(ASTNodeType(ASTNodeType::SelectionStatement), std::move(next), GET_UNUSED_NAME("selection_statement"));
                        }
                    }
                }
            }
        }
        throw_error();
    } else if (is_keyword(TokenType::Switch)) {
        if (is_punctuator('(')) {
            if (auto expr_node = is_expression()) {
                if (is_punctuator(')')) {
                    if (auto stat_node = is_statement()) {
                        next.push_back(std::move(expr_node));
                        next.push_back(std::move(stat_node));
                        return MakeNode(ASTNodeType(ASTNodeType::SelectionStatement), std::move(next), GET_UNUSED_NAME("selection_statement"));
                    }
                }
            }
        }
        throw_error();
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
                        return MakeNode(ASTNodeType(ASTNodeType::IterationStatement), std::move(next), GET_UNUSED_NAME("iteration_statement"));
                    }
                }
            }
        }
        throw_error();
    } else if (is_keyword(TokenType::Do)) {
        if (auto stat_node = is_statement()) {
            if (is_punctuator('(')) {
                if (auto expr_node = is_expression()) {
                    if (is_punctuator(')')) {
                        next.push_back(std::move(expr_node));
                        next.push_back(std::move(stat_node));
                        return MakeNode(ASTNodeType(ASTNodeType::IterationStatement), std::move(next), GET_UNUSED_NAME("iteration_statement"));
                    }
                }
            }
        }
        throw_error();
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
                                return MakeNode(ASTNodeType(ASTNodeType::IterationStatement), std::move(next), GET_UNUSED_NAME("iteration_statement"));
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
                            if (expr_node) throw_error();
                            if (expr2_node) next.push_back(std::move(expr2_node));
                            if (expr3_node) next.push_back(std::move(expr3_node));
                            next.push_back(std::move(stat_node));
                            return MakeNode(ASTNodeType(ASTNodeType::IterationStatement), std::move(next), GET_UNUSED_NAME("iteration_statement"));
                        }
                    }
                }
            }
        }
        throw_error();
    }
    return nullptr;
}

ASTNodePtr Parser::is_jump_statement() {
    std::vector<ASTNodePtr> next;
    if (is_keyword(TokenType::Goto)) {
        if (auto id = is_identifier()) {
            if (is_punctuator(';')) {
                next.push_back(std::move(id));
                return MakeNode(ASTNodeType::JumpStatement, std::move(next), GET_UNUSED_NAME("jump_statement"));
            }
        }
        throw_error();
    } else if (is_keyword(TokenType::Continue)) {
        if (is_punctuator(';')) {
            return MakeNode(ASTNodeType::JumpStatement, {}, GET_UNUSED_NAME("continue_statement"));
        }
        throw_error();
    } else if (is_keyword(TokenType::Break)) {
        if (is_punctuator(';')) {
            return MakeNode(ASTNodeType::JumpStatement, {}, GET_UNUSED_NAME("break_statement"));
        }
        throw_error();
    } else if (is_keyword(TokenType::Return)) {
        auto expr_node = is_expression();
        if (is_punctuator(';')) {
            if (expr_node) next.push_back(std::move(expr_node));
            return MakeNode(ASTNodeType::JumpStatement, {}, GET_UNUSED_NAME("return_statement"));
        }
        throw_error();
    }
    return nullptr;
}

ASTNodePtr Parser::is_expression_statement() {
    auto expr_node = is_expression();
    if (is_punctuator(';')) {
        std::vector<ASTNodePtr> next;
        next.push_back(std::move(expr_node));
        return MakeNode(ASTNodeType::Expression, std::move(next), GET_UNUSED_NAME("expression_statement"));
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
                return MakeNode(ASTNodeType::LabeledStatement, std::move(next), GET_UNUSED_NAME("labeled_statement"));
            }
        }
        throw_error();
    } else if (is_keyword(TokenType::Case)) {
        if (auto const_node = is_constant_expression()) {
            if (is_punctuator(':')) {
                if (auto stat_node = is_statement()) {
                    next.push_back(std::move(const_node));
                    next.push_back(std::move(stat_node));
                    return MakeNode(ASTNodeType::LabeledStatement, std::move(next), GET_UNUSED_NAME("labeled_statement"));
                }
            }
        }
        throw_error();
    } else if (is_keyword(TokenType::Default)) {
        if (is_punctuator(':')) {
            if (auto stat_node = is_statement()) {
                next.push_back(std::move(stat_node));
                return MakeNode(ASTNodeType::LabeledStatement, std::move(next), GET_UNUSED_NAME("labeled_statement"));
            }
        }
        throw_error();
    }
    return nullptr;
}

ASTNodePtr Parser::is_compound_statement() {
    if (is_punctuator('{')) {
        std::vector<ASTNodePtr> next;
        auto block_list = is_block_item_list();
        if (is_punctuator('}')) {
            if (block_list) next.push_back(std::move(block_list));
            return MakeNode(ASTNodeType::CompoundStatement, std::move(next), GET_UNUSED_NAME("compound_statement"));
        }
        throw_error();
    }
    return nullptr;
}

ASTNodePtr Parser::is_expression() {
    std::vector<ASTNodePtr> next;
    if (auto assi_node = is_assignment_expression()) {
        next.push_back(std::move(assi_node));
        return MakeNode(ASTNodeType::Expression, std::move(next), GET_UNUSED_NAME("expression"));
    } else if (auto expr_node = is_expression()) {
        if (is_punctuator(',')) {
            if (auto assi_node2 = is_assignment_expression()) {
                next.push_back(std::move(expr_node));
                next.push_back(std::move(assi_node2));
                return MakeNode(ASTNodeType::Expression, std::move(next), GET_UNUSED_NAME("expression"));
            }
        }
        throw_error();
    }
    return nullptr;
}

ASTNodePtr Parser::is_assignment_expression() {
    std::vector<ASTNodePtr> next;
    if (auto cond_node = is_conditional_expression()) {
        next.push_back(std::move(cond_node));
        return MakeNode(ASTNodeType::AssignmentExpression, std::move(next), GET_UNUSED_NAME("assignment_expression"));
    } else if (auto un_node = is_unary_expression()) {
        if (auto assi_oper_node = is_assignment_operator()) {
            if (auto assi_node = is_assignment_expression()) {
                next.push_back(std::move(un_node));
                next.push_back(std::move(assi_oper_node));
                next.push_back(std::move(assi_node));
                return MakeNode(ASTNodeType::AssignmentExpression, std::move(next), GET_UNUSED_NAME("assignment_expression"));
            }
        }
        throw_error();
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
                        return MakeNode(ASTNodeType::ConditionalExpression, std::move(next), GET_UNUSED_NAME("conditional_expression"));
                    }
                }
            }
            throw_error();
        }
        return MakeNode(ASTNodeType::ConditionalExpression, std::move(next), GET_UNUSED_NAME("conditional_expression"));
    }
    return nullptr;
}

ASTNodePtr Parser::is_logical_or_expression() {
    std::vector<ASTNodePtr> next;
    if (auto and_node = is_logical_and_expression()) {
        next.push_back(std::move(and_node));
        return MakeNode(ASTNodeType::LogicalOrExpression, std::move(next), GET_UNUSED_NAME("logical_or_expression"));
    } else if (auto or_node = is_logical_or_expression()) {
        if (is_keyword(TokenType::OrOp)) {
            if (auto and_node = is_logical_and_expression()) {
                next.push_back(std::move(or_node));
                next.push_back(std::move(and_node));
                return MakeNode(ASTNodeType::LogicalOrExpression, std::move(next), GET_UNUSED_NAME("logical_or_expression"));
            }
        }
        throw_error();
    }
    return nullptr;
}

ASTNodePtr Parser::is_logical_and_expression() {
    std::vector<ASTNodePtr> next;
    if (auto or_node = is_inclusive_or_expression()) {
        next.push_back(std::move(or_node));
        return MakeNode(ASTNodeType::LogicalAndExpression, std::move(next), GET_UNUSED_NAME("logical_and_expression"));
    } else if (auto and_node = is_logical_and_expression()) {
        if (is_keyword(TokenType::AndOp)) {
            if (auto or_node = is_inclusive_or_expression()) {
                next.push_back(std::move(and_node));
                next.push_back(std::move(or_node));
                return MakeNode(ASTNodeType::LogicalAndExpression, std::move(next), GET_UNUSED_NAME("logical_and_expression"));
            }
        }
        throw_error();
    }
    return nullptr;
}

ASTNodePtr Parser::is_inclusive_or_expression() {
    std::vector<ASTNodePtr> next;
    if (auto ex_node = is_exclusive_or_expression()) {
        next.push_back(std::move(ex_node));
        return MakeNode(ASTNodeType::InclusiveOrExpression, std::move(next), GET_UNUSED_NAME("inclusive_or_expression"));
    } else if (auto or_node = is_inclusive_or_expression()) {
        if (is_punctuator('|')) {
            if (auto ex_node = is_exclusive_or_expression()) {
                next.push_back(std::move(or_node));
                next.push_back(std::move(ex_node));
                return MakeNode(ASTNodeType::InclusiveOrExpression, std::move(next), GET_UNUSED_NAME("inclusive_or_expression"));
            }
        }
        throw_error();
    }
    return nullptr;
}

ASTNodePtr Parser::is_exclusive_or_expression() {
    std::vector<ASTNodePtr> next;
    if (auto and_node = is_and_expression()) {
        next.push_back(std::move(and_node));
        return MakeNode(ASTNodeType::ExclusiveOrExpression, std::move(next), GET_UNUSED_NAME("exclusive_or_expression"));
    } else if (auto ex_node = is_exclusive_or_expression()) {
        if (is_punctuator('^')) {
            if (auto and_node = is_and_expression()) {
                next.push_back(std::move(ex_node));
                next.push_back(std::move(and_node));
                return MakeNode(ASTNodeType::ExclusiveOrExpression, std::move(next), GET_UNUSED_NAME("exclusive_or_expression"));
            }
        }
        throw_error();
    }
    return nullptr;
}

ASTNodePtr Parser::is_and_expression() {
    std::vector<ASTNodePtr> next;
    if (auto eq_node = is_equality_expression()) {
        next.push_back(std::move(eq_node));
        return MakeNode(ASTNodeType::AndExpression, std::move(next), GET_UNUSED_NAME("and_expression"));
    } else if (auto and_node = is_and_expression()) {
        if (is_punctuator('&')) {
            if (auto eq_node = is_equality_expression()) {
                next.push_back(std::move(and_node));
                next.push_back(std::move(eq_node));
                return MakeNode(ASTNodeType::AndExpression, std::move(next), GET_UNUSED_NAME("and_expression"));
            }
        }
    }
    return nullptr;
}

ASTNodePtr Parser::is_equality_expression() {
    std::vector<ASTNodePtr> next;
    if (auto ret_node = is_relational_expression()) {
        next.push_back(std::move(ret_node));
        return MakeNode(ASTNodeType::EqualityExpression, std::move(next), GET_UNUSED_NAME("equality_expression"));
    } else if (auto eq_node = is_equality_expression()) {
        if (is_keyword(TokenType::EqOp) || is_keyword(TokenType::NeOp)) {
            if (auto ret_node = is_relational_expression()) {
                next.push_back(std::move(eq_node));
                next.push_back(std::move(ret_node));
                return MakeNode(ASTNodeType::EqualityExpression, std::move(next), GET_UNUSED_NAME("equality_expression"));
            }
        }
        throw_error();
    }
    return nullptr;
}

ASTNodePtr Parser::is_relational_expression() {
    std::vector<ASTNodePtr> next;
    if (auto sh_node = is_shift_expression()) {
        next.push_back(std::move(sh_node));
        return MakeNode(ASTNodeType::RelationalExpression, std::move(next), GET_UNUSED_NAME("relational_expression"));
    } else if (auto req_node = is_relational_expression()) {
        if (is_keyword(TokenType::GeOp) || is_keyword(TokenType::LeOp)
            || is_punctuator('<') || is_punctuator('>'))
        {
            if (auto sh_node = is_shift_expression()) {
                next.push_back(std::move(req_node));
                next.push_back(std::move(sh_node));
                return MakeNode(ASTNodeType::RelationalExpression, std::move(next), GET_UNUSED_NAME("relational_expression"));
            }
        }
        throw_error();
    }
    return nullptr;
}

ASTNodePtr Parser::is_struct_or_union() {
    RETURN_IF_MATCH(StructOrUnion, get_token_value(), TokenType::Struct, TokenType::Union);
}

ASTNodePtr Parser::is_type_qualifier() {
    RETURN_IF_MATCH(TypeQualifier, "type_qualifier", TokenType::Const, TokenType::Volatile, TokenType::Restrict);
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
        if (list_node) next.push_back(std::move(list_node));
        if (auto pointer_node = is_pointer()) {
            next.push_back(std::move(pointer_node));
            return MakeNode(ASTNodeType::Pointer, std::move(next), GET_UNUSED_NAME("pointer"));
        }
        return MakeNode(ASTNodeType::Pointer, std::move(next), GET_UNUSED_NAME("pointer"));
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
                    return MakeNode(ASTNodeType::DirectDeclarator, std::move(next), GET_UNUSED_NAME("direct_declarator"));
                }
            } else {
                auto id_list_node = is_identifier_list();
                if (is_punctuator(')')) {
                    if (id_list_node) next.push_back(std::move(id_list_node));
                    return MakeNode(ASTNodeType::DirectDeclarator, std::move(next), GET_UNUSED_NAME("direct_declarator"));
                }
            }
        }
        return MakeNode(ASTNodeType::DirectDeclarator, std::move(next), GET_UNUSED_NAME("direct_declarator"));
    } else if (is_punctuator('(')) {
        if (auto decl_node = is_declarator()) {
            if (is_punctuator(')')) {
                next.push_back(std::move(decl_node));
                if (is_punctuator('[')) {

                } else if (is_punctuator('(')) {
                    if (auto par_node = is_parameter_type_list()) {
                        if (is_punctuator(')')) {
                            next.push_back(std::move(par_node));
                            return MakeNode(ASTNodeType::DirectDeclarator, std::move(next), GET_UNUSED_NAME("direct_declarator"));
                        }
                    } else {
                        auto id_list_node = is_identifier_list();
                        if (is_punctuator(')')) {
                            if (id_list_node) next.push_back(std::move(id_list_node));
                            return MakeNode(ASTNodeType::DirectDeclarator, std::move(next), GET_UNUSED_NAME("direct_declarator"));
                        }
                    }
                }
                return MakeNode(ASTNodeType::DirectDeclarator, std::move(next), GET_UNUSED_NAME("direct_declarator"));
            }
        }
        throw_error();
    }
    return nullptr;
}

ASTNodePtr Parser::is_parameter_type_list() {
    std::vector<ASTNodePtr> next;
    if (auto par_node = is_parameter_list()) {
        next.push_back(std::move(par_node));
        if (is_punctuator(',')) {
            if (is_keyword(TokenType::Ellipsis)) {
                auto ellipsis = MakeNode(ASTNodeType::Ellipsis, {}, GET_UNUSED_NAME("ellipsis"));
                next.push_back(std::move(ellipsis));
                return MakeNode(ASTNodeType::ParameterTypeList, std::move(next), GET_UNUSED_NAME("parameter_type_list"));    
            }
            throw_error();
        } else {
            return MakeNode(ASTNodeType::ParameterTypeList, std::move(next), GET_UNUSED_NAME("parameter_type_list"));
        }
    }
    return nullptr;
}

ASTNodePtr Parser::is_parameter_list() {
    return nullptr;
    std::vector<ASTNodePtr> next;
    if (auto par_node = is_parameter_declaration()) {
        next.push_back(std::move(par_node));
        return MakeNode(ASTNodeType::ParameterList, std::move(next), GET_UNUSED_NAME("parameter_list"));
    } else if (auto list_node = is_parameter_list()) {
        if (is_punctuator(',')) {
            if (auto par_node2 = is_parameter_declaration()) {
                next.push_back(std::move(list_node));
                next.push_back(std::move(par_node2));
                return MakeNode(ASTNodeType::ParameterList, std::move(next), GET_UNUSED_NAME("parameter_list"));
            }
        }
        throw_error();
    }
    return nullptr;
}

ASTNodePtr Parser::is_parameter_declaration() {
    std::vector<ASTNodePtr> next;
    if (auto decls_node = is_declaration_specifiers()) {
        if (auto decl_node = is_declarator()) {
            next.push_back(std::move(decls_node));
            next.push_back(std::move(decl_node));
            return MakeNode(ASTNodeType::ParameterDeclaration, std::move(next), GET_UNUSED_NAME("parameter_declaration"));
        } else {
            next.push_back(std::move(decls_node));
            // auto abst_node = is_abstract_declarator(); TODO
            // if (abst_node) next.push_back(std::move(abst_node))
            return MakeNode(ASTNodeType::ParameterDeclaration, std::move(next), GET_UNUSED_NAME("parameter_declaration"));
        }
    }
    return nullptr;
}

ASTNodePtr Parser::is_specifier_qualifier_list() {
    auto spec_list_node = MakeNode(ASTNodeType::SpecifierQualifierList, {}, GET_UNUSED_NAME("specifier_qualifier_list"));
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
    if (auto un_node = is_unary_expression()) {
        std::vector<ASTNodePtr> next;
        next.push_back(std::move(un_node));
        return MakeNode(ASTNodeType::CastExpression, std::move(next), GET_UNUSED_NAME("cast_expression"));
    } else if (is_punctuator('(')) {
        if (auto type_node = is_type_name()) {
            if (is_punctuator(')')) {
                if (auto cast_node = is_cast_expression()) {
                    std::vector<ASTNodePtr> next;
                    next.push_back(std::move(type_node));
                    next.push_back(std::move(cast_node));
                    return MakeNode(ASTNodeType::CastExpression, std::move(next), GET_UNUSED_NAME("cast_expression"));
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
        return MakeNode(ASTNodeType::UnaryExpression, std::move(next), GET_UNUSED_NAME("unary_expression"));
    }
    return nullptr;
}

ASTNodePtr Parser::is_type_name() {
    if (auto spec_node = is_specifier_qualifier_list()) {
        std::vector<ASTNodePtr> next;
        next.push_back(std::move(spec_node));
        if (auto abs_node = is_abstract_declarator()) next.push_back(std::move(abs_node));
        return MakeNode(ASTNodeType::TypeName, std::move(next), GET_UNUSED_NAME("type_name"));
    }
    return nullptr;
}

ASTNodePtr Parser::is_abstract_declarator() {
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