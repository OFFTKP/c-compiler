#include <dispatcher/dispatcher.hxx>
#include <dispatcher/dispatcher_action.hxx>

int Dispatcher::Dispatch(std::vector<Command> commands)
{
    std::vector<Action*> actions;
    for (const auto& [type, args] : commands) {
        switch (type) {
            case CommandType::LEXER: {
                Action* action = new LexerAction(args);
                actions.push_back(action);
                break;
            }
        }
    }
    for (auto& action : actions) {
        (*action)();
    }
    for (auto& action : actions) {
        delete action;
    }
    return 0;
}