#include <boolean_evaluator/boolean_evaluator.hxx>
#include <common/qa/test_base.hxx>
#include <common/log.hxx>

class TestBooleanEvaluator : public TestBase {
    void testExpressions();
    CPPUNIT_TEST_SUITE(TestBooleanEvaluator);
    CPPUNIT_TEST(testExpressions);
    CPPUNIT_TEST_SUITE_END();
};

void TestBooleanEvaluator::testExpressions() {
    auto files = getDataFiles();
    for (const auto& path : files) {
        auto src = getSource(path);
        auto lines = getLines(src);
        for (const auto& line : lines) {
            bool expected = (line[0] == 'T');
            auto expr = line.substr(2);
            bool actual = BooleanEvaluator::Evaluate(expr);
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Errored while evaluating expression!", 0, static_cast<int>(ERROR_SIZE));
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Expression doesn't match expected result!", expected, actual);
        }
    }
}

CPPUNIT_TEST_SUITE_REGISTRATION(TestBooleanEvaluator);