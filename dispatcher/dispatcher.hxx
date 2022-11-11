#ifndef DISPATCHER_HXX
#define DISPATCHER_HXX
#include <vector>
#include <dispatcher/command.hxx>

struct Dispatcher {
    static int Dispatch(std::vector<Command> commands);
};
#endif