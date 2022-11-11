#ifndef COMMAND_HXX
#define COMMAND_HXX
#include <string>
#include <stdexcept>
#include <common/str_hash.hxx>

enum class CommandType {
    LEXER,
    NONE,
};

struct Command {
    CommandType type;
    std::string arg;
};

struct Serializer {
    static CommandType Serialize(const std::string& command) {
        switch (hash(command.c_str())) {
            case hash("--lex"):
            case hash("-l"):
                return CommandType::LEXER;
            default:
                throw std::runtime_error(std::string("Unknown command: ") + command);
        }
    }
};
#endif