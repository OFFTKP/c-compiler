#include <dispatcher/dispatcher.hxx>
#include <dispatcher/dispatcher_action.hxx>

int Dispatcher::Dispatch(std::vector<Command> commands)
{
    std::vector<std::unique_ptr<Action>> actions;
    for (const auto& command : commands) {
        // TODO: why does structured binding drop ref-qualifier?
        const std::vector<std::string>& args = command.args;
        CommandType type = command.type;
        std::unique_ptr<Action> action;
        switch (type) {
            #define DEF(type, arg_count, command_short, command_long, help, ...) case CommandType::type: { action = std::make_unique<Action##type>(args); break; }
            #include <dispatcher/command_type.def>
            #undef DEF
        }
        if (action)
            actions.push_back(std::move(action));
    }
    for (auto& action : actions) {
        (*action)();
    }
    if (Global::GetCopyOutputToClipboard()) {
        Global::copyToClipboard(ss().str());
    } else {
        std::cout << ss().str();
    }
    return 0;
}