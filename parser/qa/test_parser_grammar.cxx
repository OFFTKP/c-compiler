#define PARSER_TESTING
#include <common/qa/test_base.hxx>
#include <parser/parser.hxx>
#include <lexer/lexer.hxx>
#include <regex>

class TestParserGrammar : public TestBase {
    void testStructDeclaration();
    CPPUNIT_TEST_SUITE(TestParserGrammar);
    CPPUNIT_TEST(testStructDeclaration);
    CPPUNIT_TEST_SUITE_END();

    std::vector<Token> getTokens(std::string src);
    void assertPath(const ASTNodePtr& node, std::string path);
};

void TestParserGrammar::testStructDeclaration() {
    auto tokens = getTokens("struct my_struct { int id; };");
    Parser parser(tokens);
    auto decl_node = parser.is_declaration();
    assertPath(decl_node, "declaration/declaration_specifiers/struct_or_union_specifier[2]/struct_declaration_list/struct_declaration");
}

std::vector<Token> TestParserGrammar::getTokens(std::string src) {
    Lexer lexer(src);
    return lexer.Lex();
}

void TestParserGrammar::assertPath(const ASTNodePtr& node, std::string path) {
    auto directories = split(path, "/");
    CPPUNIT_ASSERT_GREATER(size_t(0), directories.size());
    ASTNode* cur_node = node.get();
    for (size_t i = 0; i < directories.size(); i++) {
        const auto& dir = directories[i];
        // TODO: get rid of GET_UNUSED_NAME
        auto val = std::regex_replace(cur_node->Value, std::regex(R"!((\d+?)__(.+))!"), "$02");
        std::smatch match;
        if (std::regex_match(dir, match, std::regex(R"!((\w+?)\[(.+?)\])!"))) {
            size_t index = std::atoi(match[2].str().c_str());
            CPPUNIT_ASSERT_EQUAL(match[1].str(), val);
            CPPUNIT_ASSERT_GREATER(size_t(0), index);
            CPPUNIT_ASSERT_LESS(cur_node->Next.size(), index);
            cur_node = cur_node->Next[index].get();
        } else if (val == dir) {
            if (i < directories.size() - 1) {
                CPPUNIT_ASSERT_EQUAL(size_t(1), cur_node->Next.size());
                cur_node = cur_node->Next[0].get();
            } else {
                break;
            }
        } else {
            CPPUNIT_ASSERT_MESSAGE(std::string("Failed assertion path: ") + dir, false);
        }
    }
}

CPPUNIT_TEST_SUITE_REGISTRATION(TestParserGrammar);