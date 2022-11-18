#include <dispatcher/dispatcher.hxx>
#include <common/log.hxx>

int main(int argc, char** argv) {
    if (argc <= 1)
    {
        // Ask for arguments
        Dispatcher::Dispatch({ { CommandType::VERSION }, { CommandType::ASK_ARGS } });
    }
    std::vector<Command> commands;
    CommandType current_type = CommandType::NONE;
    std::vector<std::string> current_arguments;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (current_type != CommandType::NONE) {
                int expected_arg_size = getArgSize(current_type);
                if ((expected_arg_size == current_arguments.size()) ||
                (expected_arg_size == -1 && current_arguments.size() > 0))
                {
                    // -1 indicates infinite arguments
                    commands.push_back({ current_type, current_arguments });
                }
                current_type = CommandType::NONE;
                current_arguments.clear();
            }
            current_type = Serializer::Serialize(argv[i]);
        } else {
            current_arguments.push_back(argv[i]);
        }
    }
    int expected_arg_size = getArgSize(current_type);
    if ((expected_arg_size == current_arguments.size()) ||
    (expected_arg_size == -1 && current_arguments.size() > 0))
    {
        // -1 indicates infinite arguments
        commands.push_back({ current_type, current_arguments });
    } else {
        ERROR("Wrong number of arguments - Expected:" << expected_arg_size << " Got: " << current_arguments.size());
    }
    auto ret = Dispatcher::Dispatch(commands);
    return ret;
}