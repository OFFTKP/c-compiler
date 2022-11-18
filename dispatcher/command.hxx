#ifndef COMMAND_HXX
#define COMMAND_HXX
#include <common/str_hash.hxx>
#include <dispatcher/command_type.hxx>
#include <string>
#include <stdexcept>

struct Command {
    CommandType type;
    std::vector<std::string> args;
};

struct Serializer {
    static CommandType Serialize(const std::string& command) {
        switch (hash(command.c_str())) {
            #define DEF(type, arg_count, command_short, command_long, ...) case hash(command_short): case hash(command_long): return CommandType::type;
            #include <dispatcher/command_type.def>
            #undef DEF
            default:
                throw std::runtime_error(std::string("Unknown command: ") + command);
        }
    }
};
#endif