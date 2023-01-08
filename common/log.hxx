#ifndef ERROR_HXX
#define ERROR_HXX
#include <sstream>
#include <stdexcept>
#include <common/global.hxx>
#define ERROR(text) { std::stringstream ss; ss << text; Global::GetErrors().push_back(ss.str()); }
#define WARN(text) { std::stringstream ss; ss << text; Global::GetWarnings().push_back(ss.str()); }
#define LOG(text) { std::stringstream ss; ss << text; Global::GetLog().push_back(ss.str()); }
#define ERROR_SIZE Global::GetErrors().size()
#endif