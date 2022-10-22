#include <lexer/lexer.hxx>
#include <preprocessor/preprocessor.hxx>
#include <filesystem>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <tuple>
#include <vector>

// Should be defined in cmake
#ifndef DATA_DIRECTORY
#define DATA_DIRECTORY ""
#endif
#ifndef DATA_FILES
#define DATA_FILES ""
#endif

class TestLexer : public CppUnit::TestFixture {
    std::vector<std::tuple<Token, std::string>> lexFile(std::string src);
    void lexTestFiles();
    CPPUNIT_TEST_SUITE(TestLexer);
    CPPUNIT_TEST(lexTestFiles);
    CPPUNIT_TEST_SUITE_END();
};

std::vector<std::tuple<Token, std::string>> TestLexer::lexFile(std::string src) {
    std::vector<std::tuple<Token, std::string>> ret;
    Preprocessor::Process(src);
    Lexer lexer(src);
    Token token = Token::Empty;
    while (token != Token::Eof) {
        auto [temptoken, name] = lexer.GetNextToken();
        ret.push_back({temptoken, name});
        token = temptoken;
    }
    return ret;
}

void TestLexer::lexTestFiles() {
    std::vector<std::string> files = { DATA_FILES };
    CPPUNIT_ASSERT_GREATER(size_t(0), files.size());
    for (auto& path : files) {
        std::cout << "Now testing: " << path << std::endl;
        CPPUNIT_ASSERT(std::filesystem::is_regular_file(path));
        std::ifstream ifs(path);
        CPPUNIT_ASSERT(ifs.is_open() && ifs.good());
        std::stringstream ss;
        ss << ifs.rdbuf();
        auto k = lexFile(ss.str());
        for (auto [_, name] : k) {
            std::cout << name << std::endl;
        }
        CPPUNIT_ASSERT(false);
    }
}


CPPUNIT_TEST_SUITE_REGISTRATION(TestLexer);