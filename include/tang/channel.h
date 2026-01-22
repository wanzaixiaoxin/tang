#ifndef TANG_CHANNEL_H
#define TANG_CHANNEL_H

#include <vector>
#include <mutex>
#include <atomic>
#include <utility>  // 添加std::move支持
#include <coroutine>
#include <list>

namespace tang {

// 前向声明
namespace runtime {
    void schedule(std::coroutine_handle<> handle);
}

// 通道基类
class channel_base {
public:
    virtual ~channel_base() = default;
    virtual bool is_closed() const = 0;
    virtual bool is_empty() const = 0;
    virtual bool is_full() const = 0;
};

// 协程等待器基类
class coro_waiter {
public:
    virtual ~coro_waiter() = default;
    virtual void resume() = 0;
};

// 发送等待器
template <typename T>
class send_waiter : public coro_waiter {
public:
    send_waiter(std::coroutine_handle<> handle, T&& value) 
        : handle_(handle), value_(std::move(value)) {}
    
    void resume() override {
        runtime::schedule(handle_);
    }
    
    T& get_value() {
        return value_;
    }
    
private:
    std::coroutine_handle<> handle_;
    T value_;
};

// 接收等待器
template <typename T>
class recv_waiter : public coro_waiter {
public:
    recv_waiter(std::coroutine_handle<> handle, T* value_ptr) 
        : handle_(handle), value_ptr_(value_ptr) {}
    
    void resume() override {
        runtime::schedule(handle_);
    }
    
    T* get_value_ptr() {
        return value_ptr_;
    }
    
private:
    std::coroutine_handle<> handle_;
    T* value_ptr_;
};

// 通道类前向声明
template <typename T>
class channel;

// 通道发送等待器
template <typename T>
class channel_send_awaiter {
public:
    channel_send_awaiter(channel<T>& ch, T&& value) 
        : ch_(ch), value_(std::move(value)) {}
    
    bool await_ready() {
        return ch_.try_send(value_);
    }
    
    void await_suspend(std::coroutine_handle<> handle) {
        ch_.register_send_waiter(handle, std::move(value_));
    }
    
    void await_resume() {}
    
private:
    channel<T>& ch_;
    T value_;
};

// 通道接收等待器
template <typename T>
class channel_recv_awaiter {
public:
    channel_recv_awaiter(channel<T>& ch, T& value) 
        : ch_(ch), value_(value), result_(false) {}
    
    bool await_ready() {
        return ch_.try_recv(value_);
    }
    
    void await_suspend(std::coroutine_handle<> handle) {
        ch_.register_recv_waiter(handle, &value_);
    }
    
    bool await_resume() {
        return result_;
    }
    
    void set_result(bool result) {
        result_ = result;
    }
    
private:
    channel<T>& ch_;
    T& value_;
    bool result_;
};

// 通道类
template <typename T>
class channel : public channel_base {
private:
    size_t capacity_;
    std::list<T> buffer_;  // 使用list实现FIFO
    mutable std::mutex mutex_;
    std::atomic<bool> closed_{false};
    
    // 等待者列表
    std::list<std::unique_ptr<send_waiter<T>>> send_waiters_;
    std::list<std::unique_ptr<recv_waiter<T>>> recv_waiters_;
    
public:
    // 构造函数
    explicit channel(size_t capacity = 0) : capacity_(capacity) {
    }
    
    // 禁止拷贝和移动
    channel(const channel&) = delete;
    channel& operator=(const channel&) = delete;
    channel(channel&&) = delete;
    channel& operator=(channel&&) = delete;
    
    // 析构函数
    ~channel() {
        close();
    }
    
    // 尝试发送
    bool try_send(const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (closed_) {
            return false;
        }
        
        // 如果有接收等待者，直接发送
        if (!recv_waiters_.empty()) {
            auto waiter = std::move(recv_waiters_.front());
            recv_waiters_.pop_front();
            *(waiter->get_value_ptr()) = value;
            waiter->resume();
            return true;
        }
        
        // 否则检查缓冲区是否已满
        if (capacity_ == 0 || buffer_.size() >= capacity_) {
            return false;
        }
        
        buffer_.push_back(value);
        return true;
    }
    
    bool try_send(T&& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (closed_) {
            return false;
        }
        
        // 如果有接收等待者，直接发送
        if (!recv_waiters_.empty()) {
            auto waiter = std::move(recv_waiters_.front());
            recv_waiters_.pop_front();
            *(waiter->get_value_ptr()) = std::move(value);
            waiter->resume();
            return true;
        }
        
        // 否则检查缓冲区是否已满
        if (capacity_ == 0 || buffer_.size() >= capacity_) {
            return false;
        }
        
        buffer_.push_back(std::move(value));
        return true;
    }
    
    // 发送操作符 - 阻塞发送
    channel& operator<<(T&& value) {
        while (!try_send(std::move(value))) {
            // 简单的忙等待，实际应该挂起协程
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        return *this;
    }
    
    channel& operator<<(const T& value) {
        while (!try_send(value)) {
            // 简单的忙等待，实际应该挂起协程
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        return *this;
    }
    
    // 尝试接收
    bool try_recv(T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (closed_ && buffer_.empty()) {
            return false;
        }
        
        // 如果有数据，直接接收
        if (!buffer_.empty()) {
            value = std::move(buffer_.front());
            buffer_.pop_front();
            
            // 唤醒一个发送等待者
            if (!send_waiters_.empty()) {
                auto waiter = std::move(send_waiters_.front());
                send_waiters_.pop_front();
                buffer_.push_back(std::move(waiter->get_value()));
                waiter->resume();
            }
            
            return true;
        }
        
        // 如果有发送等待者，直接接收
        if (!send_waiters_.empty()) {
            auto waiter = std::move(send_waiters_.front());
            send_waiters_.pop_front();
            value = std::move(waiter->get_value());
            waiter->resume();
            return true;
        }
        
        return false;
    }
    
    // 接收操作符 - 阻塞接收
    bool operator>>(T& value) {
        while (!try_recv(value)) {
            // 简单的忙等待，实际应该挂起协程
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        return true;
    }
    
    // 注册发送等待者
    void register_send_waiter(std::coroutine_handle<> handle, T&& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        send_waiters_.emplace_back(std::make_unique<send_waiter<T>>(handle, std::move(value)));
    }
    
    // 注册接收等待者
    void register_recv_waiter(std::coroutine_handle<> handle, T* value_ptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        recv_waiters_.emplace_back(std::make_unique<recv_waiter<T>>(handle, value_ptr));
    }
    
    // 关闭通道
    void close() {
        bool was_closed = closed_.exchange(true);
        if (was_closed) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // 唤醒所有等待者
        for (auto& waiter : send_waiters_) {
            waiter->resume();
        }
        send_waiters_.clear();
        
        for (auto& waiter : recv_waiters_) {
            waiter->resume();
        }
        recv_waiters_.clear();
    }
    
    // 检查通道状态
    bool is_closed() const override {
        return closed_.load();
    }
    
    bool is_empty() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return buffer_.empty();
    }
    
    bool is_full() const override {
        if (capacity_ == 0) {
            return true;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        return buffer_.size() >= capacity_;
    }
};

} // namespace tang

#endif // TANG_CHANNEL_H