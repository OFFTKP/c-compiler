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
    if (auto ext = is_external_declaration()) {
        if (MATCH_ANY(TokenType::Eof)) {
            std::vector<ASTNodePtr> next;
            next.push_back(std::move(ext));
            start_node_ = MakeNode(ASTNodeType::Start, std::move(next), "Start");
            return std::move(start_node_);
        }
        throw_error();
    }
    return nullptr;
}

ASTNodePtr Parser::is_external_declaration() {
    commit();
    if (auto func_node = is_function_declaration()) {
        // auto ext_node = is_external_declaration();
        std::vector<ASTNodePtr> next;
        next.push_back(std::move(func_node));
        return MakeNode(ASTNodeType::ExternalDeclaration, std::move(next), GET_UNUSED_NAME("external_declaration"));
    }
    rollback();
    return nullptr;
}

ASTNodePtr Parser::is_function_declaration() {
    if (auto type_node = is_type_specifier()) {
        if (auto id_node = is_identifier()) {
            if (auto args_node = is_function_arguments()) {
                if (auto block_node = is_code_block()) {
                    auto name = "function_" + id_node->Value;
                    name = GET_UNUSED_NAME(name);
                    std::vector<ASTNodePtr> next;
                    next.push_back(std::move(type_node));
                    next.push_back(std::move(id_node));
                    next.push_back(std::move(args_node));
                    next.push_back(std::move(block_node));
                    return MakeNode(ASTNodeType::FunctionDeclaration, std::move(next), std::move(name));
                }
            }
        }
        throw_error();
    }
    return nullptr;
}

ASTNodePtr Parser::is_type_specifier() {
    RETURN_IF_MATCH(TypeSpecifier, TokenType::Int, TokenType::Char, TokenType::Double);
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

ASTNodePtr Parser::is_punctuator(char c) {
    bool ret = (get_token_type() == TokenType::Punctuator && get_token_value()[0] == c);
    auto node = ret ? MakeNode(ASTNodeType::Punctuator, {}, get_token_value()) : nullptr;
    advance_if(ret);
    return node;
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
        next.push_back(std::move(id_node));
        return MakeNode(ASTNodeType::Argument, std::move(next), GET_UNUSED_NAME("argument"));
    }
    return nullptr;
}

ASTNodePtr Parser::is_statement_list() {
    return MakeNode(ASTNodeType::StatementList, {}, GET_UNUSED_NAME("statement_list"));
}

std::string Parser::GetUML() {
    const auto& start_node = GetStartNode();
    uml_ss_.clear();
    uml_ss_ << "@startuml\n";
    uml_impl(start_node->Next);
    uml_ss_ << "hide members\nhide circle\n";
    uml_ss_ << "@enduml\n";
    return uml_ss_.str();
}

void Parser::uml_impl(const std::vector<ASTNodePtr>& nodes) {
    for (const auto& node : nodes) {
        uml_impl(node->Next);
        uml_ss_ << "class " << node->Value << " as \"" << std::regex_replace(node->Value, std::regex("__.*"), "") << "\"\n";
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