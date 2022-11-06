#include <preprocessor/preprocessor.hxx>
#include <regex>

std::string Preprocessor::Process(const std::string& input) {
    std::string ret;
    std::regex single_line_comment("//.*"); // TODO: fix comments inside quotes
    std::regex tab("[\\t]");
    std::regex whitespace("[\\s][\\s]+");
    std::regex caret_newline("\\r\\n");
    ret = remove_comments(input);
    // input = std::regex_replace(input, single_line_comment, "");
    // input = std::regex_replace(input, tab, " ");
    // input = std::regex_replace(input, whitespace, " ");
    // input = std::regex_replace(input, caret_newline, "\n");
    return ret;
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