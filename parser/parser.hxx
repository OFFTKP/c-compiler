#ifndef PARSER_HXX
#define PARSER_HXX
#include <parser/parser_node.hxx>
#include <parser/parser_defines.hxx>
#include <string>
#include <algorithm>

class Parser {
public:
    Parser(const std::string& input);
    ~Parser();

    void Parse();
    const ASTNodePtr& GetStartNode();
    std::string GetUML();
private:
    void parse_impl();
    void uml_impl(const std::vector<ASTNodePtr>& nodes);

    // Checking functions
    ASTNodePtr is_translation_unit();
    ASTNodePtr is_external_declaration();
    ASTNodePtr is_function_specifier();
    ASTNodePtr is_function_arguments();
    ASTNodePtr is_function_declaration();
    ASTNodePtr is_function_definition();
    ASTNodePtr is_argument();
    ASTNodePtr is_argument_list();
    ASTNodePtr is_statement();
    ASTNodePtr is_statement_list();
    ASTNodePtr is_assignment_operator();
    ASTNodePtr is_type_name();
    ASTNodePtr is_type_specifier();
    ASTNodePtr is_type_qualifier();
    ASTNodePtr is_struct_or_union();
    ASTNodePtr is_declarator();
    ASTNodePtr is_direct_declarator();
    ASTNodePtr is_direct_declarator2();
    ASTNodePtr is_specifier_qualifier_list();
    ASTNodePtr is_identifier();
    ASTNodePtr is_storage_class_specifier();
    ASTNodePtr is_struct_or_union_specifier();
    ASTNodePtr is_struct_declaration();
    ASTNodePtr is_struct_declarator();
    ASTNodePtr is_constant_expression();
    ASTNodePtr is_pointer();
    ASTNodePtr is_enum_specifier();
    ASTNodePtr is_typedef_name();
    ASTNodePtr is_declaration();
    ASTNodePtr is_declaration_specifiers();
    ASTNodePtr is_init_declarator_list();
    ASTNodePtr is_block_item();
    ASTNodePtr is_labeled_statement();
    ASTNodePtr is_compound_statement();
    ASTNodePtr is_expression_statement();
    ASTNodePtr is_selection_statement();
    ASTNodePtr is_iteration_statement();
    ASTNodePtr is_jump_statement();
    ASTNodePtr is_expression();
    ASTNodePtr is_assignment_expression();
    ASTNodePtr is_conditional_expression();
    ASTNodePtr is_logical_or_expression();
    ASTNodePtr is_logical_and_expression();
    ASTNodePtr is_inclusive_or_expression();
    ASTNodePtr is_exclusive_or_expression();
    ASTNodePtr is_and_expression();
    ASTNodePtr is_equality_expression();
    ASTNodePtr is_relational_expression();
    ASTNodePtr is_cast_expression();
    ASTNodePtr is_unary_operator();
    ASTNodePtr is_unary_expression();
    ASTNodePtr is_argument_expression_list();
    ASTNodePtr is_parameter_type_list();
    ASTNodePtr is_parameter_list();
    ASTNodePtr is_parameter_declaration();
    // Left recursive
    MAKE_SIMPLE_LIST(type_specifier_list, type_specifier, TypeSpecifierList, true);
    MAKE_SIMPLE_LIST(type_qualifier_list, type_qualifier, TypeQualifierList, true);
    MAKE_SIMPLE_LIST(declaration_list, declaration, DeclarationList, true);
    MAKE_SIMPLE_LIST(block_item_list, block_item, BlockItemList, true);
    MAKE_SIMPLE_LIST(struct_declaration_list, struct_declaration, StructDeclarationList, true);
    MAKE_SIMPLE_LIST(struct_declarator_list, struct_declarator, StructDeclaratorList, is_punctuator(','));
    MAKE_SIMPLE_LIST(shift_expression, additive_expression, ShiftExpression, is_keyword(TokenType::LeftOp) || is_keyword(TokenType::RightOp));
    MAKE_SIMPLE_LIST(additive_expression, multiplicative_expression, AdditiveExpression, is_punctuator('+') || is_punctuator('-'));
    MAKE_SIMPLE_LIST(multiplicative_expression, cast_expression, MultiplicativeExpression, is_punctuator('*') || is_punctuator('/') || is_punctuator('%'));
    MAKE_SIMPLE_LIST(identifier_list, identifier, IdentifierList, is_punctuator(','));

    bool is_punctuator(char c);
    bool is_keyword(TokenType t);

    void throw_error();

    inline bool check_many(TokenType type, std::initializer_list<TokenType> many) {
        return std::ranges::find(many, type) != many.end();
    }

    TokenType get_token_type();
    std::string get_token_value();
    std::string get_uml_name(const std::string& name);
    bool advance_if(bool adv);
    void rollback();
    void commit();
    const std::string& input_;
    std::vector<Token> tokens_;
    std::vector<Token>::const_iterator index_;
    std::vector<Token>::const_iterator rollback_index_;
    ASTNodePtr start_node_;
    std::stringstream uml_ss_;
    std::unordered_map<std::string, int> value_count_ {};
};
#endif