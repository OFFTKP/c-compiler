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
    const ParserNode& GetStartNode();
private:
    void parse_impl();

    // Checking functions
    bool is_translation_unit();
    bool is_external_declaration();
    bool is_function_declaration();
    bool is_type_specifier();
    bool is_identifier();
    bool is_function_arguments();
    bool is_code_block();
    bool is_argument_list();
    bool is_argument();
    bool is_statement_list();
    bool is_statement();

    void throw_error();

    inline bool check_many(TokenType type, std::initializer_list<TokenType> many) {
        return std::ranges::find(many, type) != many.end();
    }

    TokenType get_token_type();
    std::string get_token_value();
    bool advance_if(bool adv);
    void rollback();
    void commit();
    const std::string& input_;
    std::vector<Token> tokens_;
    std::vector<Token>::const_iterator index_;
    std::vector<Token>::const_iterator rollback_index_;
    ParserNode start_node_;
    std::shared_ptr<ParserNode> current_node_;
    std::shared_ptr<ParserNode> uncommited_node_;
};
#endif