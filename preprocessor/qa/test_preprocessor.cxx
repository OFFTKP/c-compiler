#include <preprocessor/preprocessor.hxx>
#include <common/test_base.hxx>
#include <filesystem>

class TestPreprocessor : public TestBase {
    void preprocessTestFiles();
    CPPUNIT_TEST_SUITE(TestPreprocessor);
    CPPUNIT_TEST(preprocessTestFiles);
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
        bool passed = preprocessor.IsDefined("passed");
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

CPPUNIT_TEST_SUITE_REGISTRATION(TestPreprocessor);