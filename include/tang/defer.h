#ifndef TANG_DEFER_H
#define TANG_DEFER_H

#include <functional>

namespace tang {

// Defer实现类
class defer_guard {
public:
    // 构造函数，接受一个函数对象
    template <typename F>
    explicit defer_guard(F&& f) : func_(std::forward<F>(f)) {}
    
    // 移动构造函数
    defer_guard(defer_guard&& other) noexcept : func_(std::move(other.func_)) {
        other.func_ = nullptr;
    }
    
    // 禁止拷贝构造和赋值
    defer_guard(const defer_guard&) = delete;
    defer_guard& operator=(const defer_guard&) = delete;
    defer_guard& operator=(defer_guard&&) = delete;
    
    // 析构函数，执行保存的函数
    ~defer_guard() {
        if (func_) {
            func_();
        }
    }
    
private:
    // 保存的函数对象
    std::function<void()> func_;
};

} // namespace tang

// 辅助宏：生成唯一变量名
#define TANG_DEFER_CONCAT(a, b) a##b
#define TANG_DEFER_VAR_NAME(line) TANG_DEFER_CONCAT(__defer_, line)

// Defer语法糖宏
#define defer(code) \
    tang::defer_guard TANG_DEFER_VAR_NAME(__LINE__) = tang::defer_guard([&]() { code; })

#endif // TANG_DEFER_H
