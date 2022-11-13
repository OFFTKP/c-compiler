#include <preprocessor/preprocessor.hxx>
#include <common/qa/test_base.hxx>
#include <common/qa/defines.hxx>
#include <filesystem>

class TestPreprocessor : public TestBase {
    void preprocessTestFiles();
    void preprocessErrorFiles();
    CPPUNIT_TEST_SUITE(TestPreprocessor);
    CPPUNIT_TEST(preprocessTestFiles);
    CPPUNIT_TEST(preprocessErrorFiles);
    CPPUNIT_TEST_SUITE_END();
};

void TestPreprocessor::preprocessTestFiles() {
    const auto& files = getDataFiles();
    for (auto& path : files) {
        std::cout << "Now testing: " << path << std::endl;
        CPPUNIT_ASSERT(std::filesystem::is_regular_file(path));
        std::ifstream ifs(path);
        CPPUNIT_ASSERT(ifs.is_open() && ifs.good());
        std::stringstream ss;
        ss << ifs.rdbuf();
        std::string str = ss.str();
        Preprocessor preprocessor(str, path);
        preprocessor.Process();
        std::string message = "Failed file: " + path;
        bool passed = preprocessor.IsDefined(__TEST_PASSED);
        if (!passed) {
            // Dump definitions
            const auto& defines = preprocessor.GetDefines();
            int i = 1;
            if (defines.empty())
                std::cout << "Defines are empty!" << std::endl;
            else for (const auto& [key, value] : defines)
                std::cout << i++ << ". " << key << ": " << value << std::endl;
        }
        CPPUNIT_ASSERT_MESSAGE(message, passed);
    }
}

void TestPreprocessor::preprocessErrorFiles() {
    const auto& files = getErrorFiles();
    for (auto& path : files) {
        std::cout << "Now testing: " << path << std::endl;
        CPPUNIT_ASSERT(std::filesystem::is_regular_file(path));
        std::ifstream ifs(path);
        CPPUNIT_ASSERT(ifs.is_open() && ifs.good());
        std::stringstream ss;
        ss << ifs.rdbuf();
        std::string str = ss.str();
        Preprocessor preprocessor(str, path);
        try {
            preprocessor.Process();
        } catch (const std::exception& ex) {
            CPPUNIT_ASSERT(preprocessor.IsError());
            const auto& defines = preprocessor.GetDefines();
            // The first definition will define the type of test
            const auto& [key, _] = *defines.begin();
            PreprocessorError expected_error = getExpectedError(key);
            PreprocessorError actual_error = preprocessor.GetError();
            CPPUNIT_ASSERT_EQUAL_MESSAGE(
                "Error codes don't match!",
                expected_error,
                actual_error
            );
            // Errored as expected, passed
            continue;
        }
        std::string message = "File did not error: " + path;
        CPPUNIT_ASSERT_MESSAGE(message, false);
    }
}

CPPUNIT_TEST_SUITE_REGISTRATION(TestPreprocessor);