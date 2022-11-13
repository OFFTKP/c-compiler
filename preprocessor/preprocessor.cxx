#include <preprocessor/preprocessor.hxx>
#include <common/error.hxx>
#include <regex>
#include <iostream>
#include <fstream>
#include <sstream>

Preprocessor::Preprocessor(const std::string& input, std::filesystem::path current_path)
    : input_(input)
    , current_path_(current_path)
{}

Preprocessor::~Preprocessor() {}

std::string Preprocessor::Process() {
    std::string ret = input_;
    ret = process_impl(ret);
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
    return ret;
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

std::string Preprocessor::process_impl(const std::string& input) {
    std::stringstream ssret;
    std::vector<std::string> lines;
    std::stringstream stream(input);
    std::string line;
    while (std::getline(stream, line))
        lines.push_back(line);

    std::regex define_regex(R"(^[ \t]*#[ \t]*define[ \t]+([A-Za-z_][A-Za-z_0-9]+)[ \t]*(.*)$)");
    std::regex undef_regex(R"(^[ \t]*#[ \t]*undef[ \t]+([A-Za-z_][A-Za-z_0-9]+)[ \t]*$)");
    std::regex ifdef_regex(R"(^[ \t]*#[ \t]*ifdef[ \t]+([A-Za-z_][A-Za-z_0-9]+)[ \t]*$)");
    std::regex ifndef_regex(R"(^[ \t]*#[ \t]*ifndef[ \t]+([A-Za-z_][A-Za-z_0-9]+)[ \t]*$)");
    std::regex endif_regex(R"(^[ \t]*#[ \t]*endif[ \t]*$)");
    std::regex include_regex_angle(R"(^[ \t]*#[ \t]*include[ \t]+<([A-Za-z0-9_/\.]+)>[ \t]*$)");
    std::regex include_regex_quote(R"!!(^[ \t]*#[ \t]*include[ \t]+"([A-Za-z0-9_/\.]+)"[ \t]*$)!!");

    bool conditional = true;
    size_t i = 0;
    while (i < lines.size()) {
        auto line = lines[i++];
        if (conditional) {
            if (std::find(line.begin(), line.end(), '#') != line.end()) {
                // Handle lines that contain '#'
                std::smatch match;
                if (std::regex_match(line, match, define_regex)) {
                    // #define
                    defines_[match[1]] = match[2];
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
                        path = std::filesystem::path(current_path_.parent_path().string() + "/" + match[1].str());
                    else
                        path = temppath;
                    include_impl(lines, path, i);
                }
            } else {
                ssret << line;
            }
        } else {
            std::smatch match;
            if (std::regex_match(line, match, endif_regex)) {
                conditional = true;
            }
        }
    }
    return ssret.str();
}

void Preprocessor::include_impl(std::vector<std::string>& lines, std::filesystem::path path, size_t i) {
    std::stringstream ss;
    if (!std::filesystem::is_regular_file(path))
        ERROR("File not found: " << path)
    std::ifstream file(path);
    if (!file.is_open())
        ERROR("Could not resolve include: " << path)
    else
        ss << file.rdbuf();
    std::vector<std::string> include_lines;
    std::string line;
    while (std::getline(ss, line))
        include_lines.push_back(line);
    // Add include_lines to lines
    lines.insert(lines.begin() + i, include_lines.begin(), include_lines.end());
}