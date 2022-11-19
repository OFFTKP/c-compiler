#ifndef STATE_HXX
#define STATE_HXX
#include <vector>
#include <string>
#include <iostream>

// Holds the global compiler state
struct Global {
    #define VAR(type, name, default) static type& Get##name() { static type name = default; return name; }
    VAR(int, MaxIncludeDepth, 200)
    VAR(std::vector<std::string>, Errors, {})
    VAR(std::vector<std::string>, Warnings, {})
    VAR(std::vector<std::string>, Log, {})
    VAR(bool, Debug, false)
    VAR(std::string, CurrentPath, "");
    #undef VAR

    static void dumpLog() {
        auto& log = Global::GetLog();
        for (const auto& entry : log) {
            std::cout << entry << std::endl;
        }
    }

    static void dumpWarnings() {
        auto& warnings = Global::GetWarnings();
        for (const auto& entry : warnings) {
            std::cout << entry << std::endl;
        }
    }
    
    static void dumpErrors() {
        auto& errors = Global::GetErrors();
        for (const auto& entry : errors) {
            std::cout << entry << std::endl;
        }
    }

    static void showHelp() {
        #define DEF(type, arg_count, command_short, command_long, help, ...) std::cout << command_short ", " command_long << ": " << help << std::endl;
        #include <dispatcher/command_type.def>
        #undef DEF
    }
};
#endif