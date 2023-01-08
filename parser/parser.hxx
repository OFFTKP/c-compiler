#ifndef PARSER_HXX
#define PARSER_HXX
#include <parser/parser_node.hxx>
#include <parser/parser_defines.hxx>
#include <token/token.hxx>
#include <string>
#include <vector>
#include <algorithm>
#include <ranges>

class Parser {
public:
    Parser(const std::string& input);
    ~Parser();

    void Parse();
    const ASTNodePtr& GetStartNode();
    std::string GetUML();
public:
    static void parser_error();
    void parse_impl();
    void find_error();
    void simplify(), simplify_impl(ASTNodePtr& node);
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
    ASTNodePtr is_init_declarator();
    ASTNodePtr is_initializer();
    ASTNodePtr is_block_item();
    ASTNodePtr is_labeled_statement();
    ASTNodePtr is_compound_statement();
    ASTNodePtr is_expression_statement();
    ASTNodePtr is_selection_statement();
    ASTNodePtr is_iteration_statement();
    ASTNodePtr is_jump_statement();
    ASTNodePtr is_assignment_expression();
    ASTNodePtr is_conditional_expression();
    ASTNodePtr is_cast_expression();
    ASTNodePtr is_unary_operator();
    ASTNodePtr is_unary_expression();
    ASTNodePtr is_parameter_type_list();
    ASTNodePtr is_parameter_declaration();
    ASTNodePtr is_abstract_declarator();
    ASTNodePtr is_enumerator();
    ASTNodePtr is_designation();
    ASTNodePtr is_designator();
    ASTNodePtr is_enumeration_constant();
    ASTNodePtr is_primary_expression();
    ASTNodePtr is_constant();
    ASTNodePtr is_string_literal();
    ASTNodePtr is_postfix_expression(), _is_postfix_expression();
    ASTNodePtr is_logical_and_expression(), _is_logical_and_expression();
    ASTNodePtr is_logical_or_expression(), _is_logical_or_expression();
    ASTNodePtr is_or_expression(), _is_or_expression();
    ASTNodePtr is_xor_expression(), _is_xor_expression();
    ASTNodePtr is_and_expression(), _is_and_expression();
    ASTNodePtr is_equality_expression(), _is_equality_expression();
    ASTNodePtr is_relational_expression(), _is_relational_expression();
    ASTNodePtr is_direct_declarator(), _is_direct_declarator();
    ASTNodePtr is_type_qualifier_list(), _is_type_qualifier_list();
    ASTNodePtr is_declaration_list(), _is_declaration_list();
    ASTNodePtr is_block_item_list(), _is_block_item_list();
    ASTNodePtr is_struct_declaration_list(), _is_struct_declaration_list();
    ASTNodePtr is_struct_declarator_list(), _is_struct_declarator_list();
    ASTNodePtr is_init_declarator_list(), _is_init_declarator_list();
    ASTNodePtr is_parameter_list(), _is_parameter_list();
    ASTNodePtr is_identifier_list(), _is_identifier_list();
    ASTNodePtr is_shift_expression(), _is_shift_expression();
    ASTNodePtr is_additive_expression(), _is_additive_expression();
    ASTNodePtr is_multiplicative_expression(), _is_multiplicative_expression();
    ASTNodePtr is_argument_expression_list(), _is_argument_expression_list();
    ASTNodePtr is_enumerator_list(), _is_enumerator_list();
    ASTNodePtr is_initializer_list(), _is_initializer_list();
    ASTNodePtr is_designator_list(), _is_designator_list();
    ASTNodePtr is_expression(), _is_expression();
    ASTNodePtr is_assignment_expression_list(), _is_assignment_expression_list();

    // consume(...) functions are the same as is_...() functions
    // but if the token is not the expected one, it will throw an exception
    bool is_punctuator(char c);
    void consume(char c);
    bool is_keyword(TokenType t);
    void consume(TokenType t);

    inline bool check_many(TokenType type, std::initializer_list<TokenType> many) {
        return std::ranges::find(many, type) != many.end();
    }

    TokenType get_token_type();
    std::string get_token_value();
    std::string get_unique_name(std::string);
    bool advance_if(bool adv);
    bool type_defined(const std::string& type);
    const std::string& input_;
    std::vector<Token> tokens_;
    std::vector<Token>::const_iterator index_;
    ASTNodePtr start_node_;
    std::stringstream uml_ss_;
    std::unordered_map<std::string, int> uml_value_count_ {};
    std::unordered_map<std::string, std::string> typedefs_ {};
    friend class TestParserGrammar;
    friend class Dispatcher;
};
#endif