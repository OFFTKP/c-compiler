#ifndef SCOPE_GUARD_HXX
#define SCOPE_GUARD_HXX
#include <functional>

struct ScopeGuard {
    ScopeGuard(std::function<void()> func) : func_(func) {}
    ~ScopeGuard() {
        func_();
    }
private:
    std::function<void()> func_;
};
#endif