#include <lexer/lexer.hxx>
#include <preprocessor/preprocessor.hxx>
#include <common/qa/test_base.hxx>
#include <filesystem>
#include <tuple>
#include <vector>

class TestLexer : public TestBase {
    std::vector<std::tuple<Token, std::string>> lexFile(std::string src);
    void lexTestFiles();
    CPPUNIT_TEST_SUITE(TestLexer);
    CPPUNIT_TEST(lexTestFiles);
    CPPUNIT_TEST_SUITE_END();
};

std::vector<std::tuple<Token, std::string>> TestLexer::lexFile(std::string src) {
    std::vector<std::tuple<Token, std::string>> ret;
    Preprocessor preprocessor(src, "");
    src = preprocessor.Process();
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
    const auto& files = getDataFiles();
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
        // CPPUNIT_ASSERT(false);
    }
}


CPPUNIT_TEST_SUITE_REGISTRATION(TestLexer);