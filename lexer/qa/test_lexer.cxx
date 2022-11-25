#include <lexer/lexer.hxx>
#include <preprocessor/preprocessor.hxx>
#include <common/qa/test_base.hxx>
#include <filesystem>
#include <tuple>
#include <vector>
#include <fstream>

class TestLexer : public TestBase {
    std::string lexFile(std::string src);
    void lexTestFiles();
    CPPUNIT_TEST_SUITE(TestLexer);
    CPPUNIT_TEST(lexTestFiles);
    CPPUNIT_TEST_SUITE_END();
};

std::string TestLexer::lexFile(std::string src) {
    std::stringstream ss;
    std::vector<Token> tokens;
    Preprocessor preprocessor(src);
    src = preprocessor.Process();
    Lexer lexer(src);
    TokenType token = TokenType::Empty;
    while (token != TokenType::Eof) {
        auto [temptoken, name] = lexer.GetNextTokenType();
        if (temptoken != TokenType::Eof)
            tokens.push_back({temptoken, name});
        token = temptoken;
    }
    for (size_t i = 0; i < tokens.size(); i++) {
        const auto& [type, value] = tokens[i];
        ss << value << " " << static_cast<int>(type) << "\n";
    }
    std::string ret = ss.str();
    return ret;
}

void TestLexer::lexTestFiles() {
    auto src_files = getDataFiles("compare/src");
    auto expected_path = getDataPath() + "/compare/expected/";
    for (size_t i = 0; i < src_files.size(); i++) {
        auto filename = std::filesystem::path(src_files[i]).filename().string();
        auto src = getSource(src_files[i]);
        auto expected = getSource(expected_path + filename + ".e");
        auto actual = lexFile(src);
        // Output the generated file
        auto outpath = getDataPath() + "/compare/src/out/";
        std::filesystem::create_directories(outpath);
        std::ofstream ofs(outpath + filename + ".e");
        ofs << actual;
        if (expected != actual) {
            // Go line by line to check exactly what doesn't match
            auto actual_vec = getLines(actual);
            auto expected_vec = getLines(expected);
            for (size_t i = 0; i < actual_vec.size(); i++) {
                CPPUNIT_ASSERT_EQUAL_MESSAGE("Results don't match!", expected_vec[i], actual_vec[i]);
            }
        }
    }
}


CPPUNIT_TEST_SUITE_REGISTRATION(TestLexer);