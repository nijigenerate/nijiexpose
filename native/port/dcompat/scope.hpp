#pragma once
#include <exception>
#include <utility>
namespace nijiexpose::dcompat {
template<typename F> class ScopeExit {
    F fn;
public:
    explicit ScopeExit(F fn) : fn(std::move(fn)) {}
    ScopeExit(const ScopeExit&) = delete;
    ~ScopeExit() { fn(); }
};
template<typename F> class ScopeSuccess {
    F fn;
    int depth = std::uncaught_exceptions();
public:
    explicit ScopeSuccess(F fn) : fn(std::move(fn)) {}
    ScopeSuccess(const ScopeSuccess&) = delete;
    ~ScopeSuccess() noexcept(false) { if (std::uncaught_exceptions() == depth) fn(); }
};
}
