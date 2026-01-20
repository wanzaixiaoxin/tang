#ifndef TANG_CHANNEL_H
#define TANG_CHANNEL_H

#include <coroutine>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <optional>
#include <memory>

namespace tang {

// 通道基类
class channel_base {
public:
    virtual ~channel_base() = default;
    virtual bool is_closed() const = 0;
    virtual bool is_empty() const = 0;
    virtual bool is_full() const = 0;
};

// 接收等待器（前置声明）
template <typename T>
class recv_awaiter;

// 通道类
template <typename T>
class channel : public channel_base {
private:
    // 发送者信息
    struct sender_info {
        std::coroutine_handle<> handle;
        T value;
        bool is_rvalue;
    };
    
    // 接收者信息
    struct receiver_info {
        std::coroutine_handle<> handle;
        T* value_ptr;
        std::optional<T>* result_ptr;
    };
    
    size_t capacity_;
    std::vector<T> buffer_;
    size_t head_ = 0;
    std::queue<sender_info> send_queue_;
    std::queue<receiver_info> recv_queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> closed_{false};
    
    void push_sender(std::coroutine_handle<> handle, const T& value, bool is_rvalue);
    void push_receiver(std::coroutine_handle<> handle, T* value_ptr, std::optional<T>* result_ptr);
    void notify_waiters();
    
    // 发送等待器
    class send_awaiter {
    public:
        send_awaiter(channel& ch, const T& value) 
            : ch_(ch), value_(value), is_rvalue_(false) {}
        
        send_awaiter(channel& ch, T&& value) 
            : ch_(ch), value_(std::move(value)), is_rvalue_(true) {}
        
        bool await_ready() const {
            return ch_.try_send(is_rvalue_ ? std::move(const_cast<T&>(value_)) : value_);
        }
        
        void await_suspend(std::coroutine_handle<> handle) {
            if (is_rvalue_) {
                ch_.push_sender(handle, std::move(const_cast<T&>(value_)), true);
            } else {
                ch_.push_sender(handle, value_, false);
            }
        }
        
        void await_resume() {}
        
    private:
        channel& ch_;
        mutable T value_;
        bool is_rvalue_;
    };
    
    // 接收等待器（外部定义）
    friend class recv_awaiter<T>;
    
public:
    // 构造函数
    explicit channel(size_t capacity = 0) : capacity_(capacity) {
        if (capacity_ > 0) {
            buffer_.reserve(capacity_);
        }
    }
    
    // 移动构造
    channel(channel&& other) noexcept {
        std::lock_guard<std::mutex> lock(other.mutex_);
        capacity_ = other.capacity_;
        buffer_ = std::move(other.buffer_);
        head_ = other.head_;
        send_queue_ = std::move(other.send_queue_);
        recv_queue_ = std::move(other.recv_queue_);
        closed_ = other.closed_.load();
    }
    
    // 移动赋值
    channel& operator=(channel&& other) noexcept {
        if (this != &other) {
            std::lock_guard<std::mutex> lock1(mutex_, std::adopt_lock);
            std::lock_guard<std::mutex> lock2(other.mutex_, std::adopt_lock);
            std::lock(mutex_, other.mutex_);
            
            capacity_ = other.capacity_;
            buffer_ = std::move(other.buffer_);
            head_ = other.head_;
            send_queue_ = std::move(other.send_queue_);
            recv_queue_ = std::move(other.recv_queue_);
            closed_ = other.closed_.load();
        }
        return *this;
    }
    
    // 禁止拷贝
    channel(const channel&) = delete;
    channel& operator=(const channel&) = delete;
    
    // 析构函数
    ~channel() {
        close();
    }
    
    // 发送操作
    bool try_send(const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (closed_) {
            return false;
        }
        
        if (!recv_queue_.empty()) {
            auto receiver = recv_queue_.front();
            recv_queue_.pop();
            
            if (receiver.value_ptr) {
                *receiver.value_ptr = value;
            }
            if (receiver.result_ptr) {
                *receiver.result_ptr = value;
            }
            
            receiver.handle.resume();
            return true;
        }
        
        if (capacity_ == 0) {
            return false;
        }
        
        if (buffer_.size() < capacity_) {
            buffer_.push_back(value);
            return true;
        }
        
        return false;
    }
    
    bool try_send(T&& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (closed_) {
            return false;
        }
        
        if (!recv_queue_.empty()) {
            auto receiver = recv_queue_.front();
            recv_queue_.pop();
            
            if (receiver.value_ptr) {
                *receiver.value_ptr = std::move(value);
            }
            if (receiver.result_ptr) {
                *receiver.result_ptr = std::move(value);
            }
            
            receiver.handle.resume();
            return true;
        }
        
        if (capacity_ == 0) {
            return false;
        }
        
        if (buffer_.size() < capacity_) {
            buffer_.push_back(std::move(value));
            return true;
        }
        
        return false;
    }
    
    // 接收操作
    bool try_recv(T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!buffer_.empty()) {
            value = std::move(buffer_[head_]);
            head_ = (head_ + 1) % capacity_;
            buffer_.erase(buffer_.begin());
            
            if (!send_queue_.empty()) {
                auto sender = send_queue_.front();
                send_queue_.pop();
                
                if (sender.is_rvalue) {
                    buffer_.push_back(std::move(sender.value));
                } else {
                    buffer_.push_back(sender.value);
                }
                
                sender.handle.resume();
            }
            
            return true;
        }
        
        if (!send_queue_.empty()) {
            auto sender = send_queue_.front();
            send_queue_.pop();
            
            if (sender.is_rvalue) {
                value = std::move(sender.value);
            } else {
                value = sender.value;
            }
            
            sender.handle.resume();
            return true;
        }
        
        return false;
    }
    
    // 协程等待发送
    auto await_send(const T& value) {
        return send_awaiter(*this, value);
    }
    
    auto await_send(T&& value) {
        return send_awaiter(*this, std::move(value));
    }
    
    // 发送操作符
    auto operator<<(const T& value) {
        return await_send(value);
    }
    
    auto operator<<(T&& value) {
        return await_send(std::move(value));
    }
    
    // 协程等待接收
    auto await_recv(T& value) {
        return recv_awaiter<T>(*this, &value);
    }
    
    auto await_recv() {
        return recv_awaiter<T>(*this);
    }
    
    // 接收操作符
    bool operator>>(T& value) {
        return try_recv(value);
    }
    
    // 关闭通道
    void close() {
        if (!closed_.exchange(true)) {
            std::lock_guard<std::mutex> lock(mutex_);
            cv_.notify_all();
        }
    }
    
    // 检查通道状态
    bool is_closed() const override {
        return closed_.load(std::memory_order_acquire);
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

// 接收等待器实现
template <typename T>
class recv_awaiter {
public:
    recv_awaiter(channel<T>& ch, T* value_ptr = nullptr) 
        : ch_(ch), value_ptr_(value_ptr) {}
    
    bool await_ready() const {
        if (value_ptr_) {
            return ch_.try_recv(*value_ptr_);
        } else {
            T temp;
            return ch_.try_recv(temp);
        }
    }
    
    void await_suspend(std::coroutine_handle<> handle) {
        ch_.push_receiver(handle, value_ptr_, &result_);
    }
    
    std::optional<T> await_resume() {
        if (value_ptr_) {
            return std::optional<T>();
        }
        return std::move(result_);
    }
    
private:
    channel<T>& ch_;
    T* value_ptr_;
    std::optional<T> result_;
};

// 成员函数实现
template <typename T>
void channel<T>::push_sender(std::coroutine_handle<> handle, const T& value, bool is_rvalue) {
    std::lock_guard<std::mutex> lock(mutex_);
    send_queue_.push({handle, value, is_rvalue});
    
    // 如果有等待的接收者，尝试匹配
    if (!recv_queue_.empty()) {
        auto sender = send_queue_.front();
        send_queue_.pop();
        
        auto receiver = recv_queue_.front();
        recv_queue_.pop();
        
        if (sender.is_rvalue) {
            if (receiver.value_ptr) {
                *receiver.value_ptr = std::move(sender.value);
            }
            if (receiver.result_ptr) {
                *receiver.result_ptr = std::move(sender.value);
            }
        } else {
            if (receiver.value_ptr) {
                *receiver.value_ptr = sender.value;
            }
            if (receiver.result_ptr) {
                *receiver.result_ptr = sender.value;
            }
        }
        
        receiver.handle.resume();
        sender.handle.resume();
    } else {
        cv_.notify_all();
    }
}

template <typename T>
void channel<T>::push_receiver(std::coroutine_handle<> handle, T* value_ptr, std::optional<T>* result_ptr) {
    std::lock_guard<std::mutex> lock(mutex_);
    recv_queue_.push({handle, value_ptr, result_ptr});
    
    // 如果有等待的发送者，尝试匹配
    if (!send_queue_.empty()) {
        auto receiver = recv_queue_.front();
        recv_queue_.pop();
        
        auto sender = send_queue_.front();
        send_queue_.pop();
        
        if (sender.is_rvalue) {
            if (receiver.value_ptr) {
                *receiver.value_ptr = std::move(sender.value);
            }
            if (receiver.result_ptr) {
                *receiver.result_ptr = std::move(sender.value);
            }
        } else {
            if (receiver.value_ptr) {
                *receiver.value_ptr = sender.value;
            }
            if (receiver.result_ptr) {
                *receiver.result_ptr = sender.value;
            }
        }
        
        receiver.handle.resume();
        sender.handle.resume();
    } else if (!buffer_.empty()) {
        // 如果缓冲区有数据，直接发送
        auto receiver = recv_queue_.front();
        recv_queue_.pop();
        
        T value = std::move(buffer_[head_]);
        head_ = (head_ + 1) % capacity_;
        buffer_.erase(buffer_.begin());
        
        if (receiver.value_ptr) {
            *receiver.value_ptr = std::move(value);
        }
        if (receiver.result_ptr) {
            *receiver.result_ptr = std::move(value);
        }
        
        receiver.handle.resume();
    } else {
        cv_.notify_all();
    }
}

template <typename T>
void channel<T>::notify_waiters() {
    cv_.notify_all();
}

} // namespace tang

#endif // TANG_CHANNEL_H