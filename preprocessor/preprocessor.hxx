#ifndef PREPROCESSOR_HXX
#define PREPROCESSOR_HXX
#include <preprocessor/preprocessor_error.hxx>
#include <optional>
#include <vector>
#include <string>
#include <filesystem>
#include <unordered_map>

struct Preprocessor {
    Preprocessor(const std::string& input, std::filesystem::path current_path);
    ~Preprocessor();

    std::string Process();
    // Check if macro is defined at the end of processing, useful for testing
    bool IsDefined(const std::string&);
    bool IsError() { return current_error_.has_value(); }
    PreprocessorError GetError() { return *current_error_; }
    const auto& GetDefines() { return defines_; }
private:
    std::string remove_comments(const std::string& input);
    std::string process_impl(const std::string& input, std::filesystem::path current_path);
    size_t include_impl(std::vector<std::string>& lines, std::filesystem::path path, size_t i);
    void throw_error(PreprocessorError error, std::filesystem::path path, size_t line);

    const std::string& input_;
    std::unordered_map<std::string, std::string> defines_;
    const std::filesystem::path current_path_;
    int current_include_depth_ = 0;
    std::optional<PreprocessorError> current_error_ = std::nullopt;
};
#endif