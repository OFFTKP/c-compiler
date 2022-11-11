#include <dispatcher/dispatcher.hxx>

int main(int argc, char** argv) {
    if (argc <= 1)
        return 1;
    std::vector<Command> commands;
    CommandType current_type = CommandType::NONE;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            current_type = Serializer::Serialize(argv[i]);
        } else {
            commands.push_back({current_type, argv[i]});
            current_type = CommandType::NONE;
        }
    }
    return Dispatcher::Dispatch(commands);
}