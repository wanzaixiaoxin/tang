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
    recv_case(channel<T>& ch, T& value)
        : ch_(ch), value_ptr_(&value), result_ptr_(nullptr), waiter_(nullptr) {}
        
    recv_case(channel<T>& ch)
        : ch_(ch), value_ptr_(nullptr), 
          result_ptr_(std::make_unique<std::optional<T>>()), waiter_(nullptr) {}
        
    select_case_type type() const override {
        return select_case_type::recv;
    }
    
    bool try_execute() override {
        T temp;
        T* target_ptr = value_ptr_ ? value_ptr_ : &temp;
        
        if (ch_.try_recv(*target_ptr)) {
            if (result_ptr_) {
                *result_ptr_ = std::move(*target_ptr);
            }
            return true;
        }
        return false;
    }
    
    void register_waiter(std::coroutine_handle<> handle) override {
        waiter_ = handle;
        ch_.add_select_recv_waiter(handle, value_ptr_, result_ptr_.get());
    }
    
    void unregister_waiter() override {
        ch_.remove_select_recv_waiter(waiter_);
        waiter_ = nullptr;
    }
    
    // 获取结果（仅适用于无参数构造的case）
    bool has_value() const {
        return result_ptr_ && result_ptr_->has_value();
    }
    
    T value() {
        if (!result_ptr_ || !result_ptr_->has_value()) {
            throw std::runtime_error("no value available");
        }
        return std::move(result_ptr_->value());
    }
    
private:
    channel<T>& ch_;
    T* value_ptr_;
    std::unique_ptr<std::optional<T>> result_ptr_;
    std::coroutine_handle<> waiter_;
};

// 发送case
template <typename T>
class send_case : public select_case {
public:
    send_case(channel<T>& ch, const T& value)
        : ch_(ch), value_(value), is_rvalue_(false), waiter_(nullptr) {}
        
    send_case(channel<T>& ch, T&& value)
        : ch_(ch), value_(std::move(value)), is_rvalue_(true), waiter_(nullptr) {}
        
    select_case_type type() const override {
        return select_case_type::send;
    }
    
    bool try_execute() override {
        if (is_rvalue_) {
            return ch_.try_send(std::move(value_));
        } else {
            return ch_.try_send(value_);
        }
    }
    
    void register_waiter(std::coroutine_handle<> handle) override {
        waiter_ = handle;
        if (is_rvalue_) {
            ch_.add_select_send_waiter(handle, std::move(value_));
        } else {
            ch_.add_select_send_waiter(handle, value_, false);
        }
    }
    
    void unregister_waiter() override {
        ch_.remove_select_send_waiter(waiter_);
        waiter_ = nullptr;
    }
    
private:
    channel<T>& ch_;
    T value_;
    bool is_rvalue_;
    std::coroutine_handle<> waiter_;
};

// 默认case
class default_select_case : public select_case {
public:
    default_select_case() = default;
    
    select_case_type type() const override {
        return select_case_type::default_case;
    }
    
    bool try_execute() override {
        // 默认case总是可执行的
        return true;
    }
    
    void register_waiter(std::coroutine_handle<>) override {
        // 默认case不需要等待
    }
    
    void unregister_waiter() override {
        // 默认case不需要等待
    }
};

// Select协程任务
template <typename... Cases>
struct select_task {
    struct promise_type {
        std::vector<std::unique_ptr<select_case>> cases_;
        std::coroutine_handle<> continuation_;
        
        select_task get_return_object() {
            return select_task(std::coroutine_handle<promise_type>::from_promise(*this));
        }
        
        std::suspend_always initial_suspend() noexcept { return {}; }
        
        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            template <typename PromiseType>
            std::coroutine_handle<> await_suspend(std::coroutine_handle<PromiseType> handle) noexcept {
                auto& promise = handle.promise();
                if (promise.continuation_) {
                    return promise.continuation_;
                }
                return std::noop_coroutine();
            }
            void await_resume() noexcept {}
        };
        
        final_awaiter final_suspend() noexcept { return {}; }
        
        void return_void() {}
        void unhandled_exception() {}
    };
    
    using handle_type = std::coroutine_handle<promise_type>;
    
    explicit select_task(handle_type h) noexcept : handle_(h) {}
    
    select_task(select_task&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }
    
    select_task& operator=(select_task&& other) noexcept {
        if (this != &other) {
            if (handle_) handle_.destroy();
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }
    
    select_task(const select_task&) = delete;
    select_task& operator=(const select_task&) = delete;
    
    ~select_task() {
        if (handle_) handle_.destroy();
    }
    
    bool await_ready() {
        for (auto& case_ptr : handle_->promise().cases_) {
            if (case_ptr->try_execute()) {
                case_ptr->execute_callback();
                return true;
            }
        }
        return false;
    }
    
    void await_suspend(std::coroutine_handle<> cont) {
        handle_->promise().continuation_ = cont;
        for (auto& case_ptr : handle_->promise().cases_) {
            case_ptr->register_waiter(handle_);
        }
    }
    
    void await_resume() {
        for (auto& case_ptr : handle_->promise().cases_) {
            case_ptr->unregister_waiter();
        }
        
        for (auto& case_ptr : handle_->promise().cases_) {
            if (case_ptr->try_execute()) {
                case_ptr->execute_callback();
                break;
            }
        }
    }
    
    void run() {
        if (handle_ && !handle_.done()) {
            runtime::schedule(handle_);
        }
    }
    
private:
    handle_type handle_;
};

// Select函数
template <typename... Cases>
select_task<Cases...> select_impl(Cases&&... cases) {
    auto& promise = co_await std::suspend_always{};
    
    promise.cases_.reserve(sizeof...(Cases));
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
    c.set_callback([]() {});
    return c;
}

template <typename T>
auto case_recv(channel<T>& ch) {
    return recv_case<T>(ch);
}

template <typename T, typename F>
auto case_recv(channel<T>& ch, T& value, F&& callback) {
    auto c = recv_case<T>(ch, value);
    c.set_callback(std::forward<F>(callback));
    return c;
}

template <typename T, typename F>
auto case_recv(channel<T>& ch, F&& callback) {
    auto c = recv_case<T>(ch);
    c.set_callback(std::forward<F>(callback));
    return c;
}

// 辅助函数：创建发送case
template <typename T>
auto case_send(channel<T>& ch, const T& value) {
    return send_case<T>(ch, value);
}

template <typename T>
auto case_send(channel<T>& ch, T&& value) {
    return send_case<T>(ch, std::move(value));
}

template <typename T, typename F>
auto case_send(channel<T>& ch, const T& value, F&& callback) {
    auto c = send_case<T>(ch, value);
    c.set_callback(std::forward<F>(callback));
    return c;
}

template <typename T, typename F>
auto case_send(channel<T>& ch, T&& value, F&& callback) {
    auto c = send_case<T>(ch, std::move(value));
    c.set_callback(std::forward<F>(callback));
    return c;
}

// 辅助函数：创建默认case
inline auto default_case() {
    return default_select_case();
}

template <typename F>
auto default_case(F&& callback) {
    auto c = default_select_case();
    c.set_callback(std::forward<F>(callback));
    return c;
}

} // namespace tang

#endif // TANG_SELECT_H
