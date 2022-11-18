#include <boolean_evaluator/boolean_evaluator.hxx>
#include <vector>
#include <common/log.hxx>

bool BooleanEvaluator::Evaluate(const std::string& expression) {
    #define err ERROR("Syntax error while evaluating boolean expression: " << expression); return false
    std::vector<Token> tokens;
    for (auto c : expression) {
        switch (c) {
            #define push(token) tokens.push_back(token); break;
            case 'T': push(TRUE);
            case 'F': push(FALSE);
            case '&': push(AND);
            case '|': push(OR);
            case '!': push(NOT);
            case '(': push(LPAR);
            case ')': push(RPAR);
            #undef push
            default: err;
        }
    }
    while (tokens.size() > 1) {
        bool something_changed = false;
        size_t i = 2;
        if (tokens.size() == 1)
            return tokens[0];
        else if (tokens.size() == 2)
        {
            if (tokens[0] == NOT && (tokens[1] == TRUE || tokens[1] == FALSE)) {
                return !tokens[1];
            } else {
                err;
            }
        }
        for (;i < tokens.size();) {
            bool replaced = false;
            auto l = tokens[i - 2];
            auto m = tokens[i - 1];
            auto r = tokens[i];
            switch(l) {
                case TRUE:
                case FALSE: {
                    if (r == TRUE || r == FALSE) {
                        if (m == AND) {
                            tokens[i - 2] = static_cast<Token>(l && r);
                            tokens.erase(tokens.begin() + i);
                            tokens.erase(tokens.begin() + (i - 1));
                            replaced = true;
                        } else if (m == OR) {
                            tokens[i - 2] = static_cast<Token>(l || r);
                            tokens.erase(tokens.begin() + i);
                            tokens.erase(tokens.begin() + (i - 1));
                            replaced = true;
                        }
                    }
                    break;
                }
                case NOT: {
                    if (m == TRUE || m == FALSE) {
                        tokens[i - 2] = static_cast<Token>(!m);
                        tokens.erase(tokens.begin() + (i - 1));
                        replaced = true;
                    }
                    break;
                }
                case LPAR: {
                    if (m == TRUE || m == FALSE) {
                        if (r == RPAR) {
                            tokens[i - 2] = static_cast<Token>(m);
                            tokens.erase(tokens.begin() + i);
                            tokens.erase(tokens.begin() + (i - 1));
                            replaced = true;
                        }
                    }
                    break;
                }
                default: break;
            }
            if (!replaced)
                i++;
            else
                something_changed = true;
        }
        if (!something_changed) {
            err;
        }
    }
    return tokens[0];
    #undef err
}