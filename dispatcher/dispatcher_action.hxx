#ifndef DISPATCHER_ACTION_HXX
#define DISPATCHER_ACTION_HXX
#include <dispatcher/command.hxx>
#include <lexer/lexer.hxx>
#include <preprocessor/preprocessor.hxx>
#include <common/log.hxx>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#define ACTION(name, ...) struct name : public Action { ~name() = default; name(const std::string& args) : Action(args) {} void operator()() override { __VA_ARGS__ } };

struct Action
{
    Action(const std::string& args) : args_(args) {}
    virtual ~Action() = default;
    virtual void operator()() = 0;
protected:
    std::string args_;
};
#elif defined a
ACTION(LexerAction,
    if (std::filesystem::is_regular_file(args_)) {
        std::ifstream ifs(args_);
        std::stringstream ssrc;
        ssrc << ifs.rdbuf();
        std::string src = ssrc.str();
        std::vector<std::tuple<Token, std::string>> tokens;
        src = Preprocessor::Process(src);
        Lexer lexer(src);
        Token cur_token = Token::Empty;
        while (cur_token != Token::Eof) {
            auto [temptoken, name] = lexer.GetNextToken();
            tokens.push_back({temptoken, name});
            cur_token = temptoken;
        }
        for (auto& [type, name] : tokens) {
            std::cout << name << ", " << type << std::endl;
        }
    } else {
        ERROR("File not found: " << args_);
    }
)

#undef ACTION

#endif