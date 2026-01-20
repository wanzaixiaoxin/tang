#ifndef TANG_SELECT_H
#define TANG_SELECT_H

#include <tang/channel.h>
#include <coroutine>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <random>
#include <algorithm>
#include <numeric>

namespace tang {

// Select case类型
enum class select_case_type {
    recv,    // 接收操作
    send,    // 发送操作
    default_case  // 默认操作
};

// Select case基类
class select_case {
public:
    virtual ~select_case() = default;
    
    // 获取case类型
    virtual select_case_type type() const = 0;
    
    // 尝试执行case
    virtual bool try_execute() = 0;
    
    // 尝试检查是否可执行
    virtual bool try_ready() = 0;
    
    // 注册等待器
    virtual void register_waiter(std::coroutine_handle<> handle) = 0;
    
    // 取消注册等待器
    virtual void unregister_waiter() = 0;
    
    // 设置执行回调
    void set_callback(std::function<void()> callback) {
        callback_ = std::move(callback);
    }
    
    // 执行回调
    void execute_callback() {
        if (callback_) {
            callback_();
        }
    }
    
protected:
    std::function<void()> callback_;
};

// 接收case
template <typename T>
class recv_case : public select_case {
public:
    recv_case(channel<T>& ch, T& value) : ch_(ch), value_ptr_(&value) {}
    recv_case(channel<T>& ch) : ch_(ch), value_ptr_(nullptr) {}
    
    select_case_type type() const override {
        return select_case_type::recv;
    }
    
    bool try_execute() override {
        if (value_ptr_) {
            return ch_.try_recv(*value_ptr_);
        } else {
            T temp;
            return ch_.try_recv(temp);
        }
    }
    
    bool try_ready() override {
        return try_execute();
    }
    
    void register_waiter(std::coroutine_handle<> handle) override {
        // 实现等待器注册逻辑
        (void)handle; // 暂未实现
    }
    
    void unregister_waiter() override {
        // 实现等待器取消注册逻辑
    }
    
private:
    channel<T>& ch_;
    T* value_ptr_;
};

// 发送case
template <typename T>
class send_case : public select_case {
public:
    send_case(channel<T>& ch, const T& value) : ch_(ch), value_(value) {}
    send_case(channel<T>& ch, T&& value) : ch_(ch), value_(std::move(value)) {}
    
    select_case_type type() const override {
        return select_case_type::send;
    }
    
    bool try_execute() override {
        return ch_.try_send(value_);
    }
    
    bool try_ready() override {
        return try_execute();
    }
    
    void register_waiter(std::coroutine_handle<> handle) override {
        // 实现等待器注册逻辑
        (void)handle; // 暂未实现
    }
    
    void unregister_waiter() override {
        // 实现等待器取消注册逻辑
    }
    
private:
    channel<T>& ch_;
    T value_;
};

// Select任务类
template <typename... Cases>
class select_task {
public:
    struct promise_type {
        std::vector<std::unique_ptr<select_case>> cases_;
        
        select_task get_return_object() {
            return select_task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() {}
        void return_void() {}
    };
    
    using handle_type = std::coroutine_handle<promise_type>;
    
    select_task(handle_type handle) : handle_(handle) {}
    ~select_task() {
        if (handle_) {
            handle_.destroy();
        }
    }
    
    // 禁止拷贝
    select_task(const select_task&) = delete;
    select_task& operator=(const select_task&) = delete;
    
    // 允许移动
    select_task(select_task&& other) noexcept : handle_(other.handle_) {
        other.handle_ = {};
    }
    
    select_task& operator=(select_task&& other) noexcept {
        if (this != &other) {
            if (handle_) {
                handle_.destroy();
            }
            handle_ = other.handle_;
            other.handle_ = {};
        }
        return *this;
    }
    
    void run() {
        if (handle_ && !handle_.done()) {
            handle_.resume();
        }
    }
    
private:
    handle_type handle_;
};

// select_awaiter类
class select_awaiter {
public:
    select_awaiter(std::vector<std::unique_ptr<select_case>> cases) 
        : cases_(std::move(cases)) {}
    
    bool await_ready() {
        // 检查是否有case可以立即执行
        for (auto& case_ptr : cases_) {
            if (case_ptr->try_ready()) {
                selected_case_ = case_ptr.get();
                return true;
            }
        }
        return false;
    }
    
    void await_suspend(std::coroutine_handle<> handle) {
        // 注册所有case的等待者
        for (auto& case_ptr : cases_) {
            case_ptr->register_waiter(handle);
        }
        
        // 再次检查是否有case可以立即执行
        for (auto& case_ptr : cases_) {
            if (case_ptr->try_ready()) {
                selected_case_ = case_ptr.get();
                handle.resume();
                return;
            }
        }
    }
    
    select_case* await_resume() noexcept {
        // 取消注册所有case的等待者
        for (auto& case_ptr : cases_) {
            case_ptr->unregister_waiter();
        }
        
        return selected_case_;
    }
    
private:
    std::vector<std::unique_ptr<select_case>> cases_;
    select_case* selected_case_ = nullptr;
};

// Select函数
template <typename... Cases>
select_task<Cases...> select_impl(Cases&&... cases) {
    co_await std::suspend_always{};
    
    // 创建promise对象
    typename select_task<Cases...>::promise_type promise;
    promise.cases_.reserve(sizeof...(Cases));
    
    // 收集所有case
    ([&promise](auto& case_obj) {
        promise.cases_.push_back(std::make_unique<std::remove_reference_t<decltype(case_obj)>>(
            std::forward<decltype(case_obj)>(case_obj)));
    }(cases), ...);
    
    co_return;
}

template <typename... Cases>
auto select(Cases&&... cases) {
    return select_impl(std::forward<Cases>(cases)...);
}

// 协程等待select
template <typename... Cases>
auto co_select(Cases&&... cases) {
    // 转换为unique_ptr数组
    std::vector<std::unique_ptr<select_case>> case_ptrs;
    case_ptrs.reserve(sizeof...(Cases));
    
    // 收集所有case
    ([&case_ptrs](auto& case_obj) {
        case_ptrs.push_back(std::make_unique<std::remove_reference_t<decltype(case_obj)>>(
            std::forward<decltype(case_obj)>(case_obj)));
    }(cases), ...);
    
    return select_awaiter(std::move(case_ptrs));
}

// 辅助函数：创建接收case
template <typename T>
auto case_recv(channel<T>& ch, T& value) {
    auto c = recv_case<T>(ch, value);
    return c;
}

template <typename T>
auto case_recv(channel<T>& ch, T& value, std::function<void()> callback) {
    auto c = recv_case<T>(ch, value);
    c.set_callback(callback);
    return c;
}

template <typename T>
auto case_recv(channel<T>& ch) {
    return recv_case<T>(ch);
}

template <typename T>
auto case_recv(channel<T>& ch, std::function<void()> callback) {
    auto c = recv_case<T>(ch);
    c.set_callback(callback);
    return c;
}

// 辅助函数：创建发送case
template <typename T>
auto case_send(channel<T>& ch, const T& value) {
    return send_case<T>(ch, value);
}

template <typename T>
auto case_send(channel<T>& ch, const T& value, std::function<void()> callback) {
    auto c = send_case<T>(ch, value);
    c.set_callback(callback);
    return c;
}

template <typename T>
auto case_send(channel<T>& ch, T&& value) {
    return send_case<T>(ch, std::move(value));
}

template <typename T>
auto case_send(channel<T>& ch, T&& value, std::function<void()> callback) {
    auto c = send_case<T>(ch, std::move(value));
    c.set_callback(callback);
    return c;
}

// 默认case类
class default_case_class : public select_case {
public:
    default_case_class(std::function<void()> callback) : callback_(std::move(callback)) {}
    
    select_case_type type() const override {
        return select_case_type::default_case;
    }
    
    bool try_execute() override {
        if (callback_) {
            callback_();
            return true;
        }
        return false;
    }
    
    bool try_ready() override {
        return true; // 默认case总是可执行
    }
    
    void register_waiter(std::coroutine_handle<> handle) override {
        (void)handle; // 默认case不需要等待
    }
    
    void unregister_waiter() override {
        // 默认case不需要取消注册
    }
    
private:
    std::function<void()> callback_;
};

// 默认case辅助函数
auto default_case(std::function<void()> callback) {
    return default_case_class(std::move(callback));
}

} // namespace tang

#endif // TANG_SELECT_H