#include <preprocessor/preprocessor.hxx>
#include <common/log.hxx>
#include <common/state.hxx>
#include <regex>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <fstream>
#include <sstream>

Preprocessor::Preprocessor(const std::string& input, std::filesystem::path path)
    : input_(input)
    , first_file_path_(path)
{}

Preprocessor::~Preprocessor() {}

std::string Preprocessor::Process() {
    if (IsError())
        return "";

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
    current_path_ = current_path;
    std::vector<std::string> lines;
    std::stringstream stream(input);
    std::string line;
    while (std::getline(stream, line))
        lines.push_back(line);

    std::regex define_regex(R"(^[ \t]*#[ \t]*define[ \t]+([A-Za-z_][A-Za-z_0-9]+)[ \t]*(.*)$)");
    std::regex error_regex(R"(^[ \t]*#[ \t]*error[ \t]+(.+)$)");
    std::regex undef_regex(R"(^[ \t]*#[ \t]*undef[ \t]+([A-Za-z_][A-Za-z_0-9]+)[ \t]*$)");
    std::regex ifdef_regex(R"(^[ \t]*#[ \t]*ifdef[ \t]+([A-Za-z_][A-Za-z_0-9]+)[ \t]*$)");
    std::regex ifndef_regex(R"(^[ \t]*#[ \t]*ifndef[ \t]+([A-Za-z_][A-Za-z_0-9]+)[ \t]*$)");
    std::regex endif_regex(R"(^[ \t]*#[ \t]*endif[ \t]*$)");
    std::regex include_regex_angle(R"(^[ \t]*#[ \t]*include[ \t]+<([A-Za-z0-9_/\.]+)>[ \t]*$)");
    std::regex include_regex_quote(R"!!(^[ \t]*#[ \t]*include[ \t]+"([A-Za-z0-9_/\.]+)"[ \t]*$)!!");

    bool conditional = true;
    size_t i = 0;
    while (i < lines.size()) {
        current_line_ = i + 1; // 1-based
        auto line = lines[i++];
        if (conditional) {
            if (std::find(line.begin(), line.end(), '#') != line.end()) {
                // Handle lines that contain '#'
                std::smatch match;
                if (std::regex_match(line, match, define_regex)) {
                    // #define
                    defines_[match[1]] = match[2];
                } else if (std::regex_match(line, match, error_regex)) {
                    // #error
                    throw_error(PreprocessorError::Directive, match[1]);
                } else if (std::regex_match(line, match, undef_regex)) {
                    // #undef
                    defines_.erase(match[1]);
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
                out_stream_ << line << "\n";
            }
        } else {
            std::smatch match;
            if (std::regex_match(line, match, endif_regex)) {
                conditional = true;
            }
        }
    }
}

size_t Preprocessor::include_impl(std::vector<std::string>& lines, std::filesystem::path path, size_t i) {
    current_include_depth_++;
    if (current_include_depth_ > Variables::GetMaxIncludeDepth())
        throw_error(PreprocessorError::IncludeDepth);
    if (!std::filesystem::is_regular_file(path))
        throw_error(PreprocessorError::IncludeNotFound);
    std::ifstream file(path);
    std::stringstream ss;
    ss << file.rdbuf();
    std::string unprocessed_src = ss.str();
    // Processing current included file
    process_impl(unprocessed_src, path);
    current_include_depth_--;
    return 0;
}

void Preprocessor::throw_error(PreprocessorError error, std::string message) {
    std::stringstream ss;
    ss << "Preprocessor - " << current_path_.string() << ":" << current_line_ << " - ";
    switch (error) {
        case PreprocessorError::IncludeDepth: {
            ss << "Max include depth reached: " << Variables::GetMaxIncludeDepth();
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
    }
    std::string error_message;
    error_message = ss.str();
    current_error_ = error;
    ERROR(error_message)
    throw std::runtime_error(error_message);
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