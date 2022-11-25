#define PARSER_TESTING
#include <common/qa/test_base.hxx>
#include <parser/parser.hxx>
#include <lexer/lexer.hxx>
#include <regex>

#define assertPathMacro(token_string, function, path) { \
    auto tokens = getTokens(token_string); \
    Parser parser(tokens); \
    auto node = parser.function(); \
    assertPath(node, path); \
}

#define assertPathsMacro(token_string, function, ...) { \
    auto tokens = getTokens(token_string); \
    Parser parser(tokens); \
    auto node = parser.function(); \
    std::vector<std::string> paths { __VA_ARGS__ }; \
    for (const auto& path : paths) \
        assertPath(node, path); \
}

class TestParserGrammar : public TestBase {
    void testStructUnionDeclaration();
    void testCastExpression();
    void testSpecifierQualifierList();
    CPPUNIT_TEST_SUITE(TestParserGrammar);
    CPPUNIT_TEST(testStructUnionDeclaration);
    CPPUNIT_TEST(testCastExpression);
    CPPUNIT_TEST(testSpecifierQualifierList);
    CPPUNIT_TEST_SUITE_END();

    std::vector<Token> getTokens(std::string src);
    void assertPath(const ASTNodePtr& node, std::string path);
};

void TestParserGrammar::testStructUnionDeclaration() {
    assertPathsMacro(
        "struct my_struct { int id; };",
        is_declaration,
        "declaration/declaration_specifiers/struct_or_union_specifier/struct_declaration_list/struct_declaration",
        "declaration/declaration_specifiers/struct_or_union_specifier/struct"
    );
    assertPathsMacro(
        "union my_union { int id; };",
        is_declaration,
        "declaration/declaration_specifiers/struct_or_union_specifier/struct_declaration_list/struct_declaration",
        "declaration/declaration_specifiers/struct_or_union_specifier/union"
    );
}

void TestParserGrammar::testCastExpression() {
    assertPathsMacro(
        "(int)my_type",
        is_cast_expression,
        "cast_expression/cast_expression/unary_expression",
        "cast_expression/type_name"
    )
}

void TestParserGrammar::testSpecifierQualifierList() {
    assertPathsMacro(
        "const restrict int",
        is_specifier_qualifier_list,
        "specifier_qualifier_list/type_qualifier",
        "specifier_qualifier_list/specifier_qualifier_list/type_qualifier",
        "specifier_qualifier_list/specifier_qualifier_list/specifier_qualifier_list/type_specifier",
    )
    assertPathsMacro(
        "unsigned long long",
        is_specifier_qualifier_list,
        "specifier_qualifier_list/type_specifier",
        "specifier_qualifier_list/specifier_qualifier_list/type_specifier",
        "specifier_qualifier_list/specifier_qualifier_list/specifier_qualifier_list/type_specifier",
    )
}

std::vector<Token> TestParserGrammar::getTokens(std::string src) {
    Lexer lexer(src);
    return lexer.Lex();
}

void TestParserGrammar::assertPath(const ASTNodePtr& start_node, std::string path) {
    CPPUNIT_ASSERT_MESSAGE("Node is empty!", start_node.get());
    auto directories = split(path, "/");
    CPPUNIT_ASSERT_GREATER(size_t(0), directories.size());
    ASTNode* cur_node = start_node.get();
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
                if (cur_node->Next.size() == 0) {
                    CPPUNIT_ASSERT_EQUAL(size_t(1), cur_node->Next.size());
                    cur_node = cur_node->Next[0].get();
                } else {
                    bool found = false;
                    for (const auto& node : cur_node->Next) {
                        auto val = std::regex_replace(node->Value, std::regex(R"!((\d+?)__(.+))!"), "$02");
                        if (val == directories[i + 1]) {
                            cur_node = node.get();
                            found = true;
                            break;
                        }
                    }
                    CPPUNIT_ASSERT_MESSAGE(std::string("Failed assertion path: ") + directories[i + 1], found);
                }
            } else {
                // Last node
                break;
            }
        } else {
            CPPUNIT_ASSERT_MESSAGE(std::string("Failed assertion path: ") + dir, false);
        }
    }
}

CPPUNIT_TEST_SUITE_REGISTRATION(TestParserGrammar);