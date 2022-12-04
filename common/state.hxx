#ifndef STATE_HXX
#define STATE_HXX
#include <vector>
#include <string>
#include <iostream>

static std::stringstream& ss() { static std::stringstream ss; return ss; }

// Holds the global compiler state
struct Global {
    #define VAR(type, name, default) static type& Get##name() { static type name = default; return name; }
    VAR(int, MaxIncludeDepth, 200)
    VAR(std::vector<std::string>, Errors, {})
    VAR(std::vector<std::string>, Warnings, {})
    VAR(std::vector<std::string>, Log, {})
    VAR(bool, Debug, false)
    VAR(std::string, CurrentPath, "")
    VAR(std::string, OutputPath, "")
    VAR(bool, CopyOutputToClipboard, false)
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

    // Platform independent way to copy to clipboard
    static void copyToClipboard(std::string str) {
        #ifdef _WIN32
        // Windows
        #include <windows.h>
        OpenClipboard(0);
        EmptyClipboard();
        HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, str.size());
        if (!hg) {
            CloseClipboard();
            return;
        }
        memcpy(GlobalLock(hg), str.c_str(), str.size());
        GlobalUnlock(hg);
        SetClipboardData(CF_TEXT, hg);
        CloseClipboard();
        GlobalFree(hg);
        #else
        // Linux
        std::string command = "echo '" + str + "' | xclip -selection clipboard";
        system(command.c_str());
        #endif
    }
};
#endif