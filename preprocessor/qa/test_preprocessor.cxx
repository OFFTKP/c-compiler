#include <preprocessor/preprocessor.hxx>
#include <common/test_base.hxx>
#include <filesystem>

// Should be defined in cmake
#ifndef DATA_FILES
#define DATA_FILES ""
#endif

class TestPreprocessor : public TestBase {
    void preprocessTestFiles();
    CPPUNIT_TEST_SUITE(TestPreprocessor);
    CPPUNIT_TEST(preprocessTestFiles);
    CPPUNIT_TEST_SUITE_END();
};

void TestPreprocessor::preprocessTestFiles() {
    std::vector<std::string> files = { DATA_FILES };
    CPPUNIT_ASSERT_GREATER(size_t(0), files.size());
    for (auto& path : files) {
        std::cout << "Now testing: " << path << std::endl;
        CPPUNIT_ASSERT(std::filesystem::is_regular_file(path));
        std::ifstream ifs(path);
        CPPUNIT_ASSERT(ifs.is_open() && ifs.good());
        std::stringstream ss;
        ss << ifs.rdbuf();
        auto out = Preprocessor::Process(ss.str());
        std::cout << out << std::endl;
        CPPUNIT_ASSERT(false);
    }
}