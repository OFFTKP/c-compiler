#include <preprocessor/preprocessor.hxx>
#include <common/qa/test_base.hxx>
#include <common/qa/defines.hxx>
#include <common/state.hxx>
#include <filesystem>

class TestPreprocessor : public TestBase {
    void preprocessConditionalCompilationFiles();
    void preprocessErrorFiles();
    void preprocessPredefinedMacroFiles();
    void preprocessFilesWithExpected();
    CPPUNIT_TEST_SUITE(TestPreprocessor);
    CPPUNIT_TEST(preprocessConditionalCompilationFiles);
    CPPUNIT_TEST(preprocessErrorFiles);
    CPPUNIT_TEST(preprocessPredefinedMacroFiles);
    CPPUNIT_TEST(preprocessFilesWithExpected);
    CPPUNIT_TEST_SUITE_END();
};

void TestPreprocessor::preprocessConditionalCompilationFiles() {
    const auto& files = getDataFiles("conditional_compilation");
    for (auto& path : files) {
        auto str = getSource(path);
        Preprocessor preprocessor(str);
        preprocessor.Process();
        std::string message = "Failed file: " + path;
        bool passed = preprocessor.IsDefined(__TEST_PASSED);
        if (!passed) {
            Preprocessor::dumpDefines();
        }
        CPPUNIT_ASSERT_MESSAGE(message, passed);
    }
}

void TestPreprocessor::preprocessErrorFiles() {
    const auto& files = getDataFiles("must_error");
    for (auto& path : files) {
        auto str = getSource(path);
        Preprocessor preprocessor(str);
        try {
            preprocessor.Process();
        } catch (const std::exception& ex) {
            CPPUNIT_ASSERT(preprocessor.IsError());
            // The first definition will define the type of test
            std::string firstline;
            std::stringstream sstr(str);
            std::getline(sstr, firstline);
            auto key = firstline.substr(std::string("#define ").size());
            PreprocessorError expected_error = getExpectedError(key);
            PreprocessorError actual_error = preprocessor.GetError();
            CPPUNIT_ASSERT_EQUAL_MESSAGE(
                std::string("Error codes don't match: ") + path,
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

void TestPreprocessor::preprocessPredefinedMacroFiles() {
    const auto& path = getDataPath() + "/predefined_macros/";
    {
        auto cur_path = path + "date.c";
        auto str = getSource(cur_path);
        Preprocessor preprocessor(str);
        auto actual = preprocessor.Process();
        auto time = std::time(nullptr);
        auto localtime = *std::localtime(&time);
        std::stringstream ss;
        ss << '"' << std::put_time(&localtime, "%b %d %Y") << '"';
        std::string expected = ss.str();
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Dates don't match!", expected, actual);
    }
    {
        auto cur_path = path + "time.c";
        auto str = getSource(cur_path);
        Preprocessor preprocessor(str);
        auto actual = preprocessor.Process();
        auto time = std::time(nullptr);
        auto localtime = *std::localtime(&time);
        std::stringstream ss;
        ss << '"' << std::put_time(&localtime, "%H:%M:%S") << '"';
        std::string expected = ss.str();
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Times don't match!", expected, actual);
    }
    {
        auto cur_path = path + "file.c";
        auto str = getSource(cur_path);
        Preprocessor preprocessor(str);
        auto actual = preprocessor.Process();
        std::stringstream ss;
        ss << '"' << cur_path << '"';
        std::string expected = ss.str();
        CPPUNIT_ASSERT_EQUAL_MESSAGE("File paths don't match!", expected, actual);
    }
    {
        auto cur_path = path + "line.c";
        auto str = getSource(cur_path);
        Preprocessor preprocessor(str);
        auto actual = preprocessor.Process();
        std::string expected = 
            "int a = 1;\n"
            "int some_func() {\n"
            "    return 1;\n"
            "}\n"
            "int b = 3;";
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Line numbers don't match!", expected, actual);
    }
}

void TestPreprocessor::preprocessFilesWithExpected() {
    const auto& src_files = getDataFiles("compare/src");
    const auto& expected_files = getDataFiles("compare/expected");
    for (size_t i = 0; i < src_files.size(); i++) {
        auto src = getSource(src_files[i]);
        auto expected = getSource(expected_files[i]);
        Preprocessor preprocessor(src);
        try {
            auto actual = preprocessor.Process();
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Sources don't match!", expected, actual);
        } catch (const std::exception& ex) {
            preprocessor.DumpDefines();
            ERROR("Caught exception: " << ex.what());
            CPPUNIT_ASSERT_MESSAGE("Exception caught!", false);
        }
    }
}

CPPUNIT_TEST_SUITE_REGISTRATION(TestPreprocessor);