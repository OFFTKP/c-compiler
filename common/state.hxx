#ifndef STATE_HXX
#define STATE_HXX
#include <vector>
#include <string>
#include <iostream>

// Holds the global compiler state
struct Variables {
    #define VAR(type, name, default) static type& Get##name() { static type name = default; return name; }
    VAR(int, MaxIncludeDepth, 200)
    VAR(std::vector<std::string>, Errors, {})
    VAR(std::vector<std::string>, Warnings, {})
    VAR(std::vector<std::string>, Log, {})
    VAR(bool, Debug, false)
    #undef VAR

    static void dumpLog() {
        auto& log = Variables::GetLog();
        std::cout << "Dumping log:" << std::endl;
        for (const auto& entry : log) {
            std::cout << entry << std::endl;
        }
    }

    static void dumpWarnings() {
        auto& warnings = Variables::GetWarnings();
        std::cout << "Dumping warnings:" << std::endl;
        for (const auto& entry : warnings) {
            std::cout << entry << std::endl;
        }
    }
    
    static void dumpErrors() {
        auto& errors = Variables::GetErrors();
        std::cout << "Dumping errors:" << std::endl;
        for (const auto& entry : errors) {
            std::cout << entry << std::endl;
        }
    }
};
#endif