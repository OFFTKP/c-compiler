#ifndef PREPROCESSOR_HXX
#define PREPROCESSOR_HXX
#include <preprocessor/preprocessor_error.hxx>
#include <optional>
#include <vector>
#include <string>
#include <filesystem>
#include <unordered_map>

using Defines = std::unordered_map<std::string, std::string>;
using FuncDefines = std::unordered_map<std::string, std::tuple<int, std::string>>;

struct Preprocessor {
    Preprocessor(const std::string& input, std::filesystem::path path);
    ~Preprocessor();

    std::string Process();
    // Check if macro is defined at the end of processing, useful for testing
    bool IsDefined(const std::string&);
    bool IsError() { return current_error_.has_value(); }
    PreprocessorError GetError() { return *current_error_; }
    const auto& GetDefines() { return defines_; }
    const auto& GetFunctionDefines() { return function_defines_; }
    void DumpDefines();

    // Dumps latest preprocessor defines, to be used after a preprocessor instance is destructed
    static void dumpDefines();
private:
    std::string remove_comments(const std::string& input);
    void process_impl(const std::string& input, std::filesystem::path current_path);
    size_t include_impl(std::vector<std::string>& lines, std::filesystem::path path, size_t i);
    void remove_unnecessary(std::string&);
    void replace_predefined_macros(std::string&);
    void replace_macros(std::string&);
    void throw_error(PreprocessorError error, std::string message = "");
    void define(std::string key, std::string value = "");
    void define_function(std::string key, int args, std::string value);
    int handle_args(const std::string& args, std::string& value);
    void initialize_defines();
    std::string simplify_expression(const std::string&);
    static void dump_defines_impl(const Defines& defines, const FuncDefines& function_defines);
    static const std::unordered_map<std::string, std::string>& get_standard_includes();

    const std::string& input_;
    Defines defines_;
    FuncDefines function_defines_;
    const std::filesystem::path first_file_path_;
    std::filesystem::path current_path_;
    size_t current_line_ = 0;
    int current_include_depth_ = 0;
    std::stringstream out_stream_;
    std::optional<PreprocessorError> current_error_ = std::nullopt;

    static Defines& getLatestDefines() {
        static Defines latest_defines_;
        return latest_defines_;
    }
    static FuncDefines& getLatestFunctionDefines() {
        static FuncDefines latest_function_defines_;
        return latest_function_defines_;
    }
};
#endif