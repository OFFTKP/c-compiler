#ifndef ERROR_HXX
#define ERROR_HXX
#include <sstream>
#include <stdexcept>
#include <common/state.hxx>
#define ERROR(text) { std::stringstream ss; ss << text; Variables::GetErrors().push_back(ss.str()); }
#define WARN(text) { std::stringstream ss; ss << text; Variables::GetWarnings().push_back(ss.str()); }
#define LOG(text) { std::stringstream ss; ss << text; Variables::GetLog().push_back(ss.str()); }
#endif