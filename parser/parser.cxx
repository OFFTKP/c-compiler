#include <lexer/lexer.hxx>
#include <parser/parser.hxx>
#include <parser/parser_defines.hxx>
#include <preprocessor/preprocessor.hxx>
#include <common/log.hxx>

Parser::Parser(const std::string& input)
    : input_(input)
    , index_(nullptr)
    , start_node_{}
    , current_node_{}
    , uncommited_node_{}
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

const ParserNode& Parser::GetStartNode() {
    if (index_ != tokens_.end()) {
        ERROR("Parse failed before GetStartNode");
        throw_error();
    }
    return start_node_;
}

void Parser::parse_impl() {
    if (!is_translation_unit())
        throw_error();
}

bool Parser::is_translation_unit() {
    start_node_.next_nodes.push_back(std::make_shared<ParserNode>(ParserNodeType::TranslationUnit));
    current_node_ = start_node_.next_nodes[0];
    if (is_external_declaration()) {
        return MATCH_ANY(TokenType::Eof);
    } else {
        return false;
    }
}

bool Parser::is_external_declaration() {
    auto temp = std::make_shared<ParserNode>(ParserNodeType::ExternalDeclaration);
    current_node_->next_nodes.push_back(temp);
    current_node_ = temp;
    commit();
    if (is_function_declaration()) {
        if (is_external_declaration()) {
            return true;
        } else {
            return true;
        }
    }
    rollback();
    return false;
}

bool Parser::is_function_declaration() {
    return is_type_specifier() && is_identifier() && is_function_arguments() && is_code_block();
}

bool Parser::is_type_specifier() {
    return advance_if(MATCH_ANY(TokenType::Int, TokenType::Char, TokenType::Double));
}

bool Parser::is_identifier() {
    return advance_if(MATCH_ANY(TokenType::Identifier));
}

bool Parser::is_function_arguments() {
    if (advance_if(MATCH_ANY(TokenType::LPar))) {
        if (is_argument_list()) {
            if (advance_if(MATCH_ANY(TokenType::RPar))) {
                return true;
            } else {
                return false;
            }
        } else {
            return false;
        }
    } else {
        return false;
    }
}

bool Parser::is_code_block() {
    if (advance_if(MATCH_ANY(TokenType::LBra))) {
        if (!is_statement_list()) {
            return false;
        }
        if (advance_if(MATCH_ANY(TokenType::RBra))) {
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

bool Parser::is_argument_list() {
    if (is_argument()) {
        if (advance_if(MATCH_ANY(TokenType::Comma))) {
            if (is_argument_list()) {
                return true;
            } else {
                return false;
            }
        } else {
            return true;
        }
    } else {
        return true;
    }
}

bool Parser::is_argument() {
    if (is_type_specifier()) {
        if (is_identifier()) {
            return true;
        } else {
            return true;
        }
    } else {
        return false;
    }
    return false;
}

bool Parser::is_statement_list() {
    if (is_statement()) {
        if (is_statement_list()) {
            return true;
        } else {
            return false;
        }
    } else {
        // Empty statement list
        return true;
    }
}

bool Parser::is_statement() {
    return false;
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

void Parser::throw_error() {
    throw std::runtime_error("Parse error");
}

void Parser::commit() {
    rollback_index_ = index_;
    if (uncommited_node_)
        current_node_->next_nodes.push_back(uncommited_node_);
}

void Parser::rollback() {
    index_ = rollback_index_;
    uncommited_node_.reset();
}