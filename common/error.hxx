#ifndef ERROR_HXX
#define ERROR_HXX
#include <sstream>
#include <stdexcept>
#define ERROR(text) { std::stringstream ss; ss << text; throw std::runtime_error(ss.str()); }
#endif