#define PARSER_TESTING
#include <common/qa/test_base.hxx>
#include <parser/parser.hxx>
#include <lexer/lexer.hxx>

#define assertPathMacro(token_string, function, path) { \
    Parser parser(token_string); \
    auto node = parser.function(); \
    assertPath(node, path); \
}

#define assertPathsMacro(token_string, function, ...) { \
    Parser parser(token_string); \
    auto node = parser.function(); \
    std::vector<std::string> paths { __VA_ARGS__ }; \
    for (const auto& path : paths) \
        assertPath(node, path); \
}

class TestParserGrammar : public TestBase {
    void testStructUnionDeclaration();
    void testCastExpression();
    void testSpecifierQualifierList();
    void testSimpleFunctionDefinition();
    CPPUNIT_TEST_SUITE(TestParserGrammar);
    CPPUNIT_TEST(testStructUnionDeclaration);
    CPPUNIT_TEST(testCastExpression);
    CPPUNIT_TEST(testSpecifierQualifierList);
    CPPUNIT_TEST(testSimpleFunctionDefinition);
    CPPUNIT_TEST_SUITE_END();

    void assertPath(const ASTNodePtr& node, std::string path);
};

void TestParserGrammar::testStructUnionDeclaration() {
    assertPathsMacro(
        "struct my_struct { int id; };",
        is_declaration,
        "declaration/declaration_specifiers/struct_or_union_specifier/struct_declaration_list/struct_declaration",
        "declaration/declaration_specifiers/struct_or_union_specifier/struct_or_union"
    );
    assertPathsMacro(
        "union my_union { int id; };",
        is_declaration,
        "declaration/declaration_specifiers/struct_or_union_specifier/struct_declaration_list/struct_declaration",
        "declaration/declaration_specifiers/struct_or_union_specifier/struct_or_union"
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

void TestParserGrammar::testSimpleFunctionDefinition() {
    assertPathsMacro(
        "int main() { return 0; }",
        is_function_definition,
        "function_definition/declaration_specifiers",
        "function_definition/declarator"
    );
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