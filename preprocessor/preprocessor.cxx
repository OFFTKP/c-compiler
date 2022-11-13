#include <preprocessor/preprocessor.hxx>
#include <common/error.hxx>
#include <common/variables.hxx>
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
    if (IsError())
        return "";

    std::string ret = input_;
    ret = process_impl(ret, current_path_);
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

std::string Preprocessor::process_impl(const std::string& input, std::filesystem::path current_path) {
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
                    auto index_offset = include_impl(lines, path, i);
                    i += index_offset;
                } else if (std::regex_match(line, match, include_regex_quote)) {
                    // #include "..."
                    std::filesystem::path temppath(match[1].str());
                    std::filesystem::path path;
                    if (temppath.is_relative())
                        path = std::filesystem::path(current_path.parent_path().string() + "/" + match[1].str());
                    else
                        path = temppath;
                    auto index_offset = include_impl(lines, path, i);
                    i += index_offset;
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

size_t Preprocessor::include_impl(std::vector<std::string>& lines, std::filesystem::path path, size_t i) {
    current_include_depth_++;
    if (current_include_depth_ > Variables::MaxIncludeDepth)
        throw_error(PreprocessorError::IncludeDepth, path, i);
    if (!std::filesystem::is_regular_file(path))
        throw_error(PreprocessorError::IncludeNotFound, path, i);
    std::ifstream file(path);
    std::stringstream ss;
    ss << file.rdbuf();
    std::string unprocessed_src = ss.str();
    // Processing current included file
    std::string processed_src = process_impl(unprocessed_src, path);
    std::stringstream processed_stream(processed_src);
    std::vector<std::string> include_lines;
    std::string line;
    while (std::getline(processed_stream, line))
        include_lines.push_back(line);
    // Add include_lines to lines
    lines.insert(lines.begin() + i, include_lines.begin(), include_lines.end());
    current_include_depth_--;
    return include_lines.size();
}

void Preprocessor::throw_error(PreprocessorError error, std::filesystem::path path, size_t line) {
    std::stringstream ss;
    ss << path << ":" << line << " - ";
    switch (error) {
    case PreprocessorError::IncludeDepth: {
        ss << "Max include depth reached: " << Variables::MaxIncludeDepth;
        break;
    }
    case PreprocessorError::IncludeNotFound: {
        ss << "File not found!";
        break;
    }
    default:
        ERROR("Unknown PreprocessorError error code: " << (int)error)
    }
    std::string message;
    message = ss.str();
    current_error_ = error;
    ERROR(message);
}