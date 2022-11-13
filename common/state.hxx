#ifndef STATE_HXX
#define STATE_HXX
#include <vector>
#include <string>
// Holds the global compiler state
struct Variables {
    #define VAR(type, name, default) static type& Get##name() { static type name = default; return name; }
    VAR(int, MaxIncludeDepth, 200)
    VAR(std::vector<std::string>, Errors, {})
    VAR(std::vector<std::string>, Warnings, {})
    VAR(std::vector<std::string>, Log, {})
};
#endif