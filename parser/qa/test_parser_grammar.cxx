#define PARSER_TESTING
#include <common/qa/test_base.hxx>
#include <parser/parser.hxx>
#include <lexer/lexer.hxx>

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
        "Declaration/DeclarationSpecifiers/StructOrUnionSpecifier/StructDeclarationList/StructDeclaration",
        "Declaration/DeclarationSpecifiers/StructOrUnionSpecifier/StructOrUnion"
    );
    assertPathsMacro(
        "union my_union { int id; };",
        is_declaration,
        "Declaration/DeclarationSpecifiers/StructOrUnionSpecifier/StructDeclarationList/StructDeclaration",
        "Declaration/DeclarationSpecifiers/StructOrUnionSpecifier/StructOrUnion"
    );
}

void TestParserGrammar::testCastExpression() {
    assertPathsMacro(
        "(int)my_type",
        is_cast_expression,
        "CastExpression/CastExpression/UnaryExpression",
        "CastExpression/TypeName"
    )
}

void TestParserGrammar::testSpecifierQualifierList() {
    assertPathsMacro(
        "const restrict int",
        is_specifier_qualifier_list,
        "SpecifierQualifierList/TypeQualifier",
        "SpecifierQualifierList/SpecifierQualifierList/TypeQualifier",
        "SpecifierQualifierList/SpecifierQualifierList/SpecifierQualifierList/TypeSpecifier",
    )
    assertPathsMacro(
        "unsigned long long",
        is_specifier_qualifier_list,
        "SpecifierQualifierList/TypeSpecifier",
        "SpecifierQualifierList/SpecifierQualifierList/TypeSpecifier",
        "SpecifierQualifierList/SpecifierQualifierList/SpecifierQualifierList/TypeSpecifier",
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
        auto val = deserialize(cur_node->Type);
        if (val == dir) {
            if (i < directories.size() - 1) {
                bool found = false;
                for (const auto& node : cur_node->Next) {
                    auto val = deserialize(node->Type);
                    if (val == directories[i + 1]) {
                        cur_node = node.get();
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    for (const auto& node : cur_node->Next) {
                        std::cout << deserialize(node->Type) << "\n";
                    }
                    std::cout << std::endl;
                }
                CPPUNIT_ASSERT_MESSAGE(std::string("Failed assertion path: ") + directories[i + 1], found);
            } else {
                // Last node
                break;
            }
        } else {
            std::stringstream ss;
            ss << "Expected: " << dir << "\n" << "Actual: " << val << "\n";
            CPPUNIT_ASSERT_MESSAGE(ss.str(), false);
        }
    }
}

CPPUNIT_TEST_SUITE_REGISTRATION(TestParserGrammar);