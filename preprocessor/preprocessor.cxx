#include <preprocessor/preprocessor.hxx>
#include <boolean_evaluator/boolean_evaluator.hxx>
#include <common/log.hxx>
#include <common/state.hxx>
#include <regex>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <fstream>
#include <sstream>
#include <algorithm>

constexpr auto word_start = R"!!(([\W]|^))!!";
constexpr auto word_end = R"!!(([\W]|$))!!";

Preprocessor::Preprocessor(const std::string& input)
    : input_(input)
    , first_file_path_(Global::GetCurrentPath())
{}

Preprocessor::~Preprocessor() {
    getLatestDefines() = std::move(defines_);
    getLatestFunctionDefines() = std::move(function_defines_);
}

void Preprocessor::dumpDefines() {
    dump_defines_impl(getLatestDefines(), getLatestFunctionDefines());
}

void Preprocessor::DumpDefines() {
    dump_defines_impl(defines_, function_defines_);
}

void Preprocessor::dump_defines_impl(const Defines& defines, const FuncDefines& function_defines) {
    if (defines.size() == 0 && function_defines.size() == 0)
        std::cout << "No defines to dump :(" << std::endl;
    else {
        if (defines.size() != 0) {
            std::cout << "Dumping defines: " << std::endl;
            for (const auto& [key, value] : defines) {
                std::cout << key << ": " << value << std::endl;
            }
        }
        if (function_defines.size() != 0) {
            std::cout << "Dumping function defines: " << std::endl;
            for (const auto& [key, tuple] : function_defines) {
                const auto& [arg_count, value] = tuple;
                std::cout << key << "(";
                for (int i = 0; i < arg_count; i++) {
                    std::cout << "$" << i + 2;
                    if (i != arg_count - 1)
                        std::cout << ", ";
                }
                std::cout << ")" << ": " << value << ")" << std::endl;
            }
        }
    }
}

std::string Preprocessor::Process() {
    if (IsError())
        return "";
    initialize_defines();
    std::string ret = input_;
    current_path_ = std::filesystem::path(first_file_path_);
    process_impl(ret, first_file_path_);
    // std::regex single_line_comment("//.*"); // TODO: fix comments inside quotes
    // std::regex tab("[\\t]");
    // std::regex whitespace("[\\s][\\s]+");
    // std::regex caret_newline("\\r\\n");
    // ret = resolve_includes(input);
    // ret = remove_comments(input);
    // input = std::regex_replace(input, single_line_comment, "");
    // input = std::regex_replace(input, tab, " ");
    // input = std::regex_replace(input, whitespace, " ");
    // input = std::regex_replace(input, caret_newline, "\n");
    return out_stream_.str();
}

bool Preprocessor::IsDefined(const std::string& macro) {
    return defines_.find(macro) != defines_.end();
}

std::string Preprocessor::remove_comments(const std::string& input) {
    using index = std::string::size_type;
    std::string ret;
    ret.reserve(input.size());
    bool is_string = false;
    bool is_comment = false;
    bool is_multiline = false;
    bool first_star = false;
    bool first_slash = false;
    for (index i = 0; i < input.size(); i++)
    {
        switch (input[i])
        {
            case '/': {
                if (!is_string) {
                    if (is_multiline && first_star) {
                        is_multiline = false;
                        first_star = false;
                    } else if (!first_slash) {
                        is_comment = true;
                        first_slash = false;
                    }
                    else
                        first_slash = true;
                }
                break;
            }
            case '*': {
                if (!is_string) {
                    if (!is_multiline && first_slash && !is_comment) {
                        is_multiline = true;
                        first_slash = false;
                    } else if (is_multiline) {
                        first_star = true;
                    }
                }
                break;
            }
            case '"': {
                if (!is_comment && !is_multiline)
                    is_string ^= true;
                [[fallthrough]];
            }
            default: {
                if (!is_comment && !is_multiline)
                    ret += input[i];
                first_star = false;
                first_slash = false;
                break;
            }
        }
    }
    return ret;
}

void Preprocessor::process_impl(const std::string& input, std::filesystem::path current_path) {
    std::string copy = input;
    remove_unnecessary(copy);
    current_path_ = current_path;
    std::vector<std::string> lines;
    std::stringstream stream(copy);
    std::string line;
    while (std::getline(stream, line))
        lines.push_back(line);

    static std::regex define_function_regex(R"!!(^[ \t]*#[ \t]*define[ \t]+([A-Za-z_][A-Za-z_0-9]*)[ \t]*\((.+?)\)[ \t]*(.*)$)!!");
    static std::regex define_regex(R"!(^[ \t]*#[ \t]*define[ \t]+([A-Za-z_][A-Za-z_0-9]*)[ \t]*(.*)$)!");
    static std::regex error_regex(R"(^[ \t]*#[ \t]*error[ \t]+(.+)$)");
    static std::regex undef_regex(R"(^[ \t]*#[ \t]*undef[ \t]+([A-Za-z_][A-Za-z_0-9]*)[ \t]*$)");
    static std::regex ifdef_regex(R"(^[ \t]*#[ \t]*ifdef[ \t]+([A-Za-z_][A-Za-z_0-9]*)[ \t]*$)");
    static std::regex ifndef_regex(R"(^[ \t]*#[ \t]*ifndef[ \t]+([A-Za-z_][A-Za-z_0-9]*)[ \t]*$)");
    static std::regex if_regex(R"!(^[ \t]*#[ \t]*if[ \t]+(.*)$)!");
    static std::regex endif_regex(R"(^[ \t]*#[ \t]*endif[ \t]*$)");
    static std::regex include_regex_angle(R"(^[ \t]*#[ \t]*include[ \t]+<([A-Za-z0-9_/\.]+)>[ \t]*$)");
    static std::regex include_regex_quote(R"!!(^[ \t]*#[ \t]*include[ \t]+"([A-Za-z0-9_/\.]+)"[ \t]*$)!!");

    bool conditional = true;
    size_t i = 0;
    while (i < lines.size()) {
        current_line_ = i + 1; // 1-based
        auto line = lines[i++];
        bool last = i == lines.size();
        if (conditional) {
            if (std::find(line.begin(), line.end(), '#') != line.end()) {
                // Handle lines that contain '#'
                std::smatch match;
                if (std::regex_match(line, match, define_function_regex)) {
                    // #define some_func()
                    std::string args = match[3];
                    int count = handle_args(match[2], args);
                    define_function(match[1], count, args);
                } else if (std::regex_match(line, match, define_regex)) {
                    // #define
                    define(match[1], match[2]);
                } else if (std::regex_match(line, match, error_regex)) {
                    // #error
                    throw_error(PreprocessorError::Directive, match[1]);
                } else if (std::regex_match(line, match, if_regex)) {
                    // #if
                    std::string unprocessed_expr = match[1];
                    auto expr = simplify_expression(unprocessed_expr);
                    conditional = BooleanEvaluator::Evaluate(expr);
                } else if (std::regex_match(line, match, undef_regex)) {
                    // #undef
                    defines_.erase(match[1]);
                    function_defines_.erase(match[1]);
                } else if (std::regex_match(line, match, ifdef_regex)) {
                    // #ifdef
                    conditional = (defines_.find(match[1]) != defines_.end());
                } else if (std::regex_match(line, match, ifndef_regex)) {
                    // #ifndef
                    conditional = (defines_.find(match[1]) == defines_.end());
                } else if (std::regex_match(line, match, include_regex_angle)) {
                    // #include <...>
                    std::filesystem::path path(std::string("/usr/include/") + match[1].str());
                    include_impl(lines, path, i);
                } else if (std::regex_match(line, match, include_regex_quote)) {
                    // #include "..."
                    std::filesystem::path temppath(match[1].str());
                    std::filesystem::path path;
                    if (temppath.is_relative())
                        path = std::filesystem::path(current_path.parent_path().string() + "/" + match[1].str());
                    else
                        path = temppath;
                    include_impl(lines, path, i);
                }
            } else {
                replace_predefined_macros(line);
                replace_macros(line);
                out_stream_ << line;
                if (!last)
                    out_stream_ << "\n";
            }
        } else {
            std::smatch match;
            if (std::regex_match(line, match, endif_regex)) {
                conditional = true;
            }
        }
    }
}

std::string Preprocessor::simplify_expression(const std::string& expression) {
    std::string ret = expression;
    std::string prefix = R"!(defined[ \t]*[\( ][ \t]*)!";
    std::string suffix = R"!([ \t]*[\) ])!";
    for (const auto& [key, _] : defines_) {
        std::regex regex(prefix + key + suffix);
        ret = std::regex_replace(ret, regex, "T");
    }
    for (const auto& [key, _] : function_defines_) {
        std::regex regex(prefix + key + suffix);
        ret = std::regex_replace(ret, regex, "T");
    }
    std::regex regex(prefix + "(.+?)" + suffix);
    ret = std::regex_replace(ret, regex, "F");
    // Remove spaces
    ret = std::regex_replace(ret, std::regex("[ \t]"), "");
    // Replace && with & and || with |
    ret = std::regex_replace(ret, std::regex(R"(\|\|)"), "|");
    ret = std::regex_replace(ret, std::regex(R"(&&)"), "&");
    return ret;
}

size_t Preprocessor::include_impl(std::vector<std::string>& lines, std::filesystem::path path, size_t i) {
    current_include_depth_++;
    if (current_include_depth_ > Global::GetMaxIncludeDepth())
        throw_error(PreprocessorError::IncludeDepth);
    if (!std::filesystem::is_regular_file(path))
        throw_error(PreprocessorError::IncludeNotFound);
    std::ifstream file(path);
    std::stringstream ss;
    ss << file.rdbuf();
    std::string unprocessed_src = ss.str();
    // Processing current included file
    process_impl(unprocessed_src, path);
    out_stream_ << '\n';
    current_include_depth_--;
    return 0;
}

void Preprocessor::throw_error(PreprocessorError error, std::string message) {
    std::stringstream ss;
    ss << "Preprocessor - " << current_path_.string() << ":" << current_line_ << " - ";
    switch (error) {
        case PreprocessorError::IncludeDepth: {
            ss << "Max include depth reached: " << Global::GetMaxIncludeDepth();
            break;
        }
        case PreprocessorError::IncludeNotFound: {
            ss << "File not found";
            break;
        }
        case PreprocessorError::Directive: {
            ss << "Error directive hit: " << message;
            break;
        }
        default: {
            ss << message;
            break;
        }
    }
    std::string error_message;
    error_message = ss.str();
    current_error_ = error;
    ERROR(error_message)
    throw std::runtime_error(error_message);
}

void Preprocessor::remove_unnecessary(std::string& input) {
    input = std::regex_replace(input, std::regex(R"(\\\n)"), "");
}

void Preprocessor::replace_predefined_macros(std::string& line) {
    static std::regex date_regex(R"!!(([\W]|^)(__DATE__)([\W]|$))!!");
    static std::regex time_regex(R"!!(([\W]|^)(__TIME__)([\W]|$))!!");
    static std::regex file_regex(R"!!(([\W]|^)(__FILE__)([\W]|$))!!");
    static std::regex line_regex(R"!!(([\W]|^)(__LINE__)([\W]|$))!!");
    auto time = std::time(nullptr);
    auto localtime = *std::localtime(&time);
    std::stringstream sdate;
    sdate << "$01" << '"' << std::put_time(&localtime, "%b %d %Y") << '"' << "$03";
    std::stringstream stime;
    stime << "$01" << '"' << std::put_time(&localtime, "%H:%M:%S") << '"' << "$03";
    std::string sline = std::string("$01") + std::to_string(current_line_) + std::string("$03");
    std::string sfile = std::string("$01\"") + current_path_.string() + std::string("\"$03");
    line = std::regex_replace(line, date_regex, sdate.str());
    line = std::regex_replace(line, time_regex, stime.str());
    line = std::regex_replace(line, file_regex, sfile);
    line = std::regex_replace(line, line_regex, sline);
}

void Preprocessor::replace_macros(std::string& line) {
    bool any_replaced = true;
    while (any_replaced) {
        any_replaced = false;
        for (const auto& [key, value] : defines_) {
            auto regex = std::regex(word_start + key + word_end);
            if (std::regex_search(line, regex)) {
                any_replaced = true;
                line = std::regex_replace(line, regex, std::string("$01") + value + std::string("$02"));
            }
        }
        const std::string spacing_opt = R"!!([ \t]*)!!";
        const std::string word = R"!!((.+?))!!";
        for (const auto& [key, tuple] : function_defines_) {
            const auto& [arg_count, value] = tuple;
            // Build the regex to replace the function
            std::stringstream regex_builder;
            regex_builder << word_start << key;
            regex_builder << spacing_opt << "\\(" << spacing_opt;
            for (int i = 0; i < arg_count; i++) {
                regex_builder << word << spacing_opt;
                if (i != arg_count - 1)
                    regex_builder << "," << spacing_opt;
            }
            regex_builder << "\\)";
            auto regex = std::regex(regex_builder.str());
            if (std::regex_search(line, regex)) {
                any_replaced = true;
                // TODO: make it not need $1? aka whatever character was before the function_macro
                line = std::regex_replace(line, regex, std::string("$01") + value);
            }
        }
    }
}

void Preprocessor::define(std::string key, std::string value) {
    if (defines_.find(key) != defines_.end())
        WARN(key << " redefinition")
    defines_[key] = value;
}

void Preprocessor::define_function(std::string key, int arg_count, std::string value) {
    if (function_defines_.find(key) != function_defines_.end())
        WARN(key << " function redefinition")
    function_defines_[key] = { arg_count, value };
}

int Preprocessor::handle_args(const std::string& args, std::string& value) {
    std::vector<std::string> arg_split;
    std::string cur_arg;
    for (int i = 0; i < args.size(); i++) {
        switch (args[i]) {
            case ',': {
                if (std::find(arg_split.begin(), arg_split.end(), cur_arg) != arg_split.end()) {
                    throw_error(PreprocessorError::Placeholder, "function macro argument redefinition: " + cur_arg);
                }
                arg_split.push_back(cur_arg);
                cur_arg = "";
                break;
            }
            case ' ':
            case '\t': {
                break;
            }
            default: {
                cur_arg += args[i];
                break;
            }
        }
    }
    arg_split.push_back(cur_arg);

    int i = 1;
    for (const auto& arg : arg_split) {
        // TODO: handle #arg and ##arg and args inside quotes
        auto regex_string = word_start + std::string("(") + arg + ")" + std::string(word_end);
        std::regex regex(regex_string);
        value = std::regex_replace(value, regex, "$1$$" + std::to_string(i + 1) + "$3");
        ++i;
    }
    return arg_split.size();
}

void Preprocessor::initialize_defines() {
    define("NULL", "0");
    if (!Global::GetDebug())
        define("NDEBUG");
    define("__STDC__", "1");
}

const std::unordered_map<std::string, std::string>& Preprocessor::get_standard_includes() {
    static std::unordered_map<std::string, std::string> includes {
        #define DEF(include, path) { include, path },
        #include <preprocessor/standard_paths.txt>
        #undef DEF
    };
    return includes;
}