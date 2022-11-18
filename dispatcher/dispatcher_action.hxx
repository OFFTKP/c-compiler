#ifndef DISPATCHER_ACTION_HXX
#define DISPATCHER_ACTION_HXX
#include <preprocessor/preprocessor.hxx>
#include <dispatcher/command.hxx>
#include <lexer/lexer.hxx>
#include <common/strings.hxx>
#include <common/log.hxx>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#define ACTION(name, ...) struct name : public Action { ~name() = default; name(const std::vector<std::string>& args) : Action(args) {} void operator()() override { __VA_ARGS__ } };

struct Action
{
    Action(const std::vector<std::string>& args) : args_(args) {}
    virtual ~Action() = default;
    virtual void operator()() = 0;
protected:
    const std::vector<std::string>& args_;
};

#define DEF(type, arg_count, command_short, command_long, ...)  ACTION(Action##type, __VA_ARGS__)
#include <dispatcher/command_type.def>
#undef DEF

#undef ACTION
#endif