#ifndef PREPROCESSOR_ERROR_HXX
#define PREPROCESSOR_ERROR_HXX
#include <common/str_hash.hxx>
#include <string>

enum class PreprocessorError {
    IncludeDepth,
    IncludeNotFound,

    Placeholder,
};

// Converts a define like __TEST_ERROR_INCLUDE_DEPTH to the expected error code
static PreprocessorError getExpectedError(std::string define) {
    switch (hash(define.c_str())) {
        case hash("__TEST_ERROR_INCLUDE_DEPTH"): return PreprocessorError::IncludeDepth;
        case hash("__TEST_ERROR_NOT_FOUND"): return PreprocessorError::IncludeNotFound;
        default: return PreprocessorError::Placeholder;
    }
}
#endif