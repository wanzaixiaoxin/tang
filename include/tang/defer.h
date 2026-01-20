#ifndef TANG_DEFER_H
#define TANG_DEFER_H

#include <functional>

namespace tang {

class defer_guard {
public:
    template <typename F>
    explicit defer_guard(F&& f) : func_(std::forward<F>(f)) {}
    
    defer_guard(defer_guard&& other) noexcept : func_(std::move(other.func_)) {
        other.func_ = nullptr;
    }
    
    defer_guard(const defer_guard&) = delete;
    defer_guard& operator=(const defer_guard&) = delete;
    defer_guard& operator=(defer_guard&&) = delete;
    
    ~defer_guard() {
        if (func_) {
            func_();
        }
    }
    
private:
    std::function<void()> func_;
};

} // namespace tang

#define TANG_DEFER_CONCAT(a, b) a##b
#define TANG_DEFER_VAR_NAME(line) TANG_DEFER_CONCAT(__defer_, line)

#define defer \
    tang::defer_guard TANG_DEFER_VAR_NAME(__LINE__) = tang::defer_guard([&]() {

#define end_defer });

#endif // TANG_DEFER_H
