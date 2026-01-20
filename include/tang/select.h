#ifndef TANG_SELECT_H
#define TANG_SELECT_H

#include <tang/channel.h>
#include <coroutine>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <random>

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
        T*& target_ptr = value_ptr_ ? value_ptr_ : &temp;
        
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
        ch_.push_receiver(handle, value_ptr_, result_ptr_.get());
    }
    
    void unregister_waiter() override {
        // 简化实现：实际需要更复杂的机制来取消注册
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
        ch_.push_sender(handle, value_, is_rvalue_);
    }
    
    void unregister_waiter() override {
        // 简化实现：实际需要更复杂的机制来取消注册
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

// Select等待器
class select_awaiter {
public:
    select_awaiter(std::vector<std::unique_ptr<select_case>>&& cases)
        : cases_(std::move(cases)), selected_case_(nullptr) {}
        
    bool await_ready() noexcept {
        // 尝试执行所有case
        for (auto& case_ptr : cases_) {
            if (case_ptr->try_execute()) {
                selected_case_ = case_ptr.get();
                return true;
            }
        }
        return false;
    }
    
    void await_suspend(std::coroutine_handle<> handle) {
        // 注册等待器到所有case
        for (auto& case_ptr : cases_) {
            case_ptr->register_waiter(handle);
        }
    }
    
    select_case* await_resume() {
        // 取消注册所有等待器
        for (auto& case_ptr : cases_) {
            case_ptr->unregister_waiter();
        }
        
        // 查找已就绪的case
        for (auto& case_ptr : cases_) {
            if (case_ptr->try_execute()) {
                selected_case_ = case_ptr.get();
                break;
            }
        }
        
        return selected_case_;
    }
    
private:
    std::vector<std::unique_ptr<select_case>> cases_;
    select_case* selected_case_;
};

// Select函数
template <typename... Cases>
void select(Cases&&... cases) {
    // 转换为unique_ptr数组
    std::vector<std::unique_ptr<select_case>> case_ptrs;
    case_ptrs.reserve(sizeof...(Cases));
    
    // 检查是否有默认case
    bool has_default = false;
    
    // 尝试执行所有case
    ([&]() {
        if (cases.type() == select_case_type::default_case) {
            has_default = true;
        }
        
        if (cases.try_execute()) {
            cases.execute_callback();
            // 抛出异常来中断后续case的执行
            throw std::runtime_error("case executed");
        }
    }(), ...);
    
    // 如果有默认case，执行默认case
    if (has_default) {
        ([&]() {
            if (cases.type() == select_case_type::default_case) {
                cases.execute_callback();
                throw std::runtime_error("default case executed");
            }
        }(), ...);
    }
    
    // 没有就绪的case，等待直到有case就绪
    // 简化实现：这里使用busy waiting，实际应该使用更高效的等待机制
    std::random_device rd;
    std::mt19937 rng(rd());
    
    while (true) {
        // 随机打乱case顺序，实现公平调度
        std::vector<size_t> indices = {0, 1, ..., sizeof...(Cases) - 1};
        std::shuffle(indices.begin(), indices.end(), rng);
        
        // 尝试执行打乱后的case
        ([&, &indices = indices](size_t i) {
            auto case_tuple = std::make_tuple(&cases...);
            auto case_ptr = std::get<i>(case_tuple);
            if (case_ptr->try_execute()) {
                case_ptr->execute_callback();
                throw std::runtime_error("case executed");
            }
        }(indices[i]), ...);
        
        // 短暂睡眠，避免CPU占用过高
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
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
    return recv_case<T>(ch, value);
}

template <typename T>
auto case_recv(channel<T>& ch) {
    return recv_case<T>(ch);
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

// 辅助函数：创建默认case
inline auto default_case() {
    return default_select_case();
}

} // namespace tang

#endif // TANG_SELECT_H
