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
    
    // 设置关联的select_awaiter
    void set_awaiter(void* awaiter) {
        awaiter_ = awaiter;
    }
    
    // 获取关联的select_awaiter
    void* get_awaiter() const {
        return awaiter_;
    }
    
protected:
    std::function<void()> callback_;
    void* awaiter_ = nullptr; // 关联的select_awaiter
    std::coroutine_handle<> handle_; // 等待的协程句柄
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
            if (ch_.try_recv(*value_ptr_)) {
                execute_callback();
                return true;
            }
        } else {
            T temp;
            if (ch_.try_recv(temp)) {
                execute_callback();
                return true;
            }
        }
        return false;
    }
    
    bool try_ready() override {
        // 仅检查是否可执行，不执行实际操作
        return !ch_.is_empty() || ch_.is_closed();
    }
    
    void register_waiter(std::coroutine_handle<> handle) override {
        handle_ = handle;
        // 注册到通道的接收等待者列表
        // 注意：这里需要通道支持select等待者注册
    }
    
    void unregister_waiter() override {
        handle_ = nullptr;
        // 从通道的接收等待者列表中移除
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
        if (ch_.try_send(value_)) {
            execute_callback();
            return true;
        }
        return false;
    }
    
    bool try_ready() override {
        // 仅检查是否可执行，不执行实际操作
        return !ch_.is_full() || ch_.is_closed();
    }
    
    void register_waiter(std::coroutine_handle<> handle) override {
        handle_ = handle;
        // 注册到通道的发送等待者列表
        // 注意：这里需要通道支持select等待者注册
    }
    
    void unregister_waiter() override {
        handle_ = nullptr;
        // 从通道的发送等待者列表中移除
    }
    
private:
    channel<T>& ch_;
    T value_;
};

// select_awaiter类
class select_awaiter {
public:
    select_awaiter(std::vector<std::unique_ptr<select_case>> cases) 
        : cases_(std::move(cases)) {
        // 为每个case设置关联的awaiter
        for (auto& case_ptr : cases_) {
            case_ptr->set_awaiter(this);
        }
    }
    
    bool await_ready() {
        // 随机打乱顺序，实现公平性
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(cases_.begin(), cases_.end(), g);
        
        // 检查是否有case可以立即执行
        for (auto& case_ptr : cases_) {
            if (case_ptr->try_ready()) {
                selected_case_ = case_ptr.get();
                selected_case_->try_execute();
                return true;
            }
        }
        return false;
    }
    
    void await_suspend(std::coroutine_handle<> handle) {
        handle_ = handle;
        
        // 注册所有case的等待者
        for (auto& case_ptr : cases_) {
            case_ptr->register_waiter(handle);
        }
        
        // 再次检查是否有case可以立即执行（防止竞态条件）
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(cases_.begin(), cases_.end(), g);
        
        for (auto& case_ptr : cases_) {
            if (case_ptr->try_ready()) {
                selected_case_ = case_ptr.get();
                selected_case_->try_execute();
                
                // 取消注册所有case的等待者
                for (auto& case_ptr : cases_) {
                    case_ptr->unregister_waiter();
                }
                
                handle.resume();
                return;
            }
        }
    }
    
    void await_resume() noexcept {
        // 取消注册所有case的等待者
        for (auto& case_ptr : cases_) {
            case_ptr->unregister_waiter();
        }
        
        // 执行选中case的回调已在try_execute中完成
    }
    
    // 唤醒等待的协程
    void wake_up(select_case* selected_case) {
        selected_case_ = selected_case;
        selected_case_->try_execute();
        runtime::schedule(handle_);
    }
    
private:
    std::vector<std::unique_ptr<select_case>> cases_;
    select_case* selected_case_ = nullptr;
    std::coroutine_handle<> handle_;
};

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

// 简化的select函数，直接执行select逻辑
template <typename... Cases>
void select(Cases&&... cases) {
    // 收集所有case到vector中
    std::vector<std::unique_ptr<select_case>> case_ptrs;
    case_ptrs.reserve(sizeof...(Cases));
    
    // 收集所有case
    ([&case_ptrs](auto& case_obj) {
        case_ptrs.push_back(std::make_unique<std::remove_reference_t<decltype(case_obj)>>(
            std::forward<decltype(case_obj)>(case_obj)));
    }(cases), ...);
    
    // 随机打乱顺序，实现公平性
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(case_ptrs.begin(), case_ptrs.end(), g);
    
    // 尝试立即执行case
    for (auto& case_ptr : case_ptrs) {
        if (case_ptr->try_ready()) {
            case_ptr->try_execute();
            return;
        }
    }
    
    // 如果没有立即可执行的case，检查是否有默认case
    for (auto& case_ptr : case_ptrs) {
        if (case_ptr->type() == select_case_type::default_case) {
            case_ptr->try_execute();
            return;
        }
    }
    
    // 如果没有默认case，忙等待直到有case可执行
    while (true) {
        // 随机打乱顺序，实现公平性
        std::shuffle(case_ptrs.begin(), case_ptrs.end(), g);
        
        for (auto& case_ptr : case_ptrs) {
            if (case_ptr->try_ready()) {
                case_ptr->try_execute();
                return;
            }
        }
        
        // 短暂睡眠，减少CPU占用
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
}

// 辅助函数：创建接收case
template <typename T>
auto case_recv(channel<T>& ch, T& value) {
    return recv_case<T>(ch, value);
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
        }
        return true;
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