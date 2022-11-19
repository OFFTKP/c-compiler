#ifndef COMMAND_TYPE_HXX
#define COMMAND_TYPE_HXX
enum class CommandType {
    NONE,
    #define DEF(type, arg_count, command_short, command_long, help, ...) type,
    #include <dispatcher/command_type.def>
    #undef DEF
};

static int getArgSize(CommandType type) {
    switch (type) {
        #define DEF(type, arg_count, command_short, command_long, help, ...) case CommandType::type: return arg_count;
        #include <dispatcher/command_type.def>
        #undef DEF
        default: return 0;
    }
}
#endif