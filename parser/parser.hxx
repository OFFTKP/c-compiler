#ifndef PARSER_HXX
#define PARSER_HXX
#include <parser/parser_node.hxx>
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
    ASTNodePtr is_code_block();
    ASTNodePtr is_argument();
    ASTNodePtr is_argument_list();
    ASTNodePtr is_statement();
    ASTNodePtr is_statement_list();
    ASTNodePtr is_assignment_operator();
    ASTNodePtr is_unary_operator();
    ASTNodePtr is_unary_expression();
    ASTNodePtr is_cast_expression();
    ASTNodePtr is_type_name();
    ASTNodePtr is_type_specifier();
    ASTNodePtr is_type_qualifier();
    ASTNodePtr is_type_qualifier_list();
    ASTNodePtr is_struct_or_union();
    ASTNodePtr is_declarator();
    ASTNodePtr is_direct_declarator();
    ASTNodePtr is_specifier_qualifier_list();
    ASTNodePtr is_identifier();
    ASTNodePtr is_punctuator(char c);

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