#include <preprocessor/preprocessor.hxx>
#include <regex>

void Preprocessor::Process(std::string& input) {
std::regex single_line_comment("//.*");
    std::regex tab("[\\t]");
    std::regex whitespace("[\\s][\\s]+");
    std::regex caret_newline("\\r\\n");
    input = std::regex_replace(input, single_line_comment, "");
    input = std::regex_replace(input, tab, " ");
    input = std::regex_replace(input, whitespace, " ");
    input = std::regex_replace(input, caret_newline, "\n");
}