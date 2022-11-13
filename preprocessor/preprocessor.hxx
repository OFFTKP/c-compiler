#ifndef PREPROCESSOR_HXX
#define PREPROCESSOR_HXX
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
    const auto& GetDefines() { return defines_; }
private:
    std::string remove_comments(const std::string& input);
    std::string process_impl(const std::string& input);
    void include_impl(std::vector<std::string>& lines, std::filesystem::path path, size_t i);

    const std::string& input_;
    std::unordered_map<std::string, std::string> defines_;
    const std::filesystem::path current_path_;
};
#endif