#ifndef BOOLEAN_EVALUATOR_HXX
#define BOOLEAN_EVALUATOR_HXX
#include <string>

struct BooleanEvaluator {
    static bool Evaluate(const std::string& expression);
private:
    enum TokenType {
        FALSE = 0,
        TRUE = 1,
        OR,
        AND,
        NOT,
        LPAR,
        RPAR,
    };
};
#endif