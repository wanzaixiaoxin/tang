#ifndef TANG_CHANNEL_H
#define TANG_CHANNEL_H

#include <coroutine>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <optional>
#include <stdexcept>

namespace tang {

// 前向声明
class channel_base {
public:
    virtual ~channel_base() = default;
    virtual bool is_closed() const = 0;
    virtual bool is_empty() const = 0;
    virtual bool is_full() const = 0;
};

// Channel类
template <typename T>
class channel : public channel_base {
public:
    // 发送等待器
    class send_awaiter {
    public:
        send_awaiter(channel& ch, const T& value)
            : ch_(ch), value_(value), is_rvalue_(false) {}
            
        send_awaiter(channel& ch, T&& value)
            : ch_(ch), value_(std::move(value)), is_rvalue_(true) {}
            
        bool await_ready() noexcept {
            return !ch_.is_full() && !ch_.is_closed();
        }
        
        void await_suspend(std::coroutine_handle<> handle) {
            ch_.push_sender(handle, value_, is_rvalue_);
        }
        
        void await_resume() {
            if (ch_.is_closed()) {
                throw std::runtime_error("send on closed channel");
            }
        }
        
    private:
        channel& ch_;
        T value_;
        bool is_rvalue_;
    };
    
    // 接收等待器
    class recv_awaiter {
    public:
        recv_awaiter(channel& ch, T& value)
            : ch_(ch), value_ptr_(&value), result_ptr_(nullptr) {}
            
        recv_awaiter(channel& ch)
            : ch_(ch), value_ptr_(nullptr), result_ptr_(std::make_unique<std::optional<T>>()) {}
            
        bool await_ready() noexcept {
            return !ch_.is_empty();
        }
        
        void await_suspend(std::coroutine_handle<> handle) {
            ch_.push_receiver(handle, value_ptr_, result_ptr_.get());
        }
        
        bool await_resume() {
            if (result_ptr_) {
                return result_ptr_->has_value();
            }
            return !ch_.is_closed() || !ch_.is_empty();
        }
        
        // 获取结果（仅适用于无参数构造的等待器）
        T value() {
            if (!result_ptr_ || !result_ptr_->has_value()) {
                throw std::runtime_error("no value available");
            }
            return std::move(result_ptr_->value());
        }
        
    private:
        channel& ch_;
        T* value_ptr_;
        std::unique_ptr<std::optional<T>> result_ptr_;
    };
    
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
    
    // 构造函数
    explicit channel(size_t capacity = 0);
    
    // 移动构造
    channel(channel&& other) noexcept;
    
    // 移动赋值
    channel& operator=(channel&& other) noexcept;
    
    // 禁止拷贝
    channel(const channel&) = delete;
    channel& operator=(const channel&) = delete;
    
    // 析构函数
    ~channel();
    
    // 发送操作
    bool try_send(const T& value);
    bool try_send(T&& value);
    void send(const T& value);
    void send(T&& value);
    
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
    
    // 接收操作
    bool try_recv(T& value);
    T recv();
    
    // 协程等待接收
    auto await_recv(T& value) {
        return recv_awaiter(*this, value);
    }
    
    auto await_recv() {
        return recv_awaiter(*this);
    }
    
    // 接收操作符
    auto operator>>(T& value) {
        return await_recv(value);
    }
    
    // 关闭通道
    void close();
    
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
    
    // 获取容量
    size_t capacity() const {
        return capacity_;
    }
    
    // 获取当前大小
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return buffer_.size();
    }
    
private:
    // 实现细节
    void push_sender(std::coroutine_handle<> handle, const T& value, bool is_rvalue);
    void push_receiver(std::coroutine_handle<> handle, T* value_ptr, std::optional<T>* result_ptr);
    
    // 唤醒等待的发送者或接收者
    void notify_waiters();
    
    // 容量（0表示无缓冲）
    size_t capacity_;
    
    // 缓冲区
    std::vector<T> buffer_;
    size_t head_ = 0;  // 缓冲区头部索引
    size_t tail_ = 0;  // 缓冲区尾部索引
    
    // 关闭状态
    std::atomic_bool closed_;
    
    // 发送等待队列
    std::queue<sender_info> send_queue_;
    
    // 接收等待队列
    std::queue<receiver_info> recv_queue_;
    
    // 互斥锁和条件变量
    mutable std::mutex mutex_;
    std::condition_variable cv_;
};

// Channel类实现
template <typename T>
channel<T>::channel(size_t capacity)
    : capacity_(capacity), closed_(false), head_(0), tail_(0)
{
    if (capacity_ > 0) {
        buffer_.resize(capacity_);
    }
}

template <typename T>
channel<T>::channel(channel&& other) noexcept
    : capacity_(other.capacity_),
      closed_(other.closed_.load(std::memory_order_acquire)),
      head_(other.head_),
      tail_(other.tail_)
{
    std::lock_guard<std::mutex> lock(other.mutex_);
    buffer_ = std::move(other.buffer_);
    send_queue_ = std::move(other.send_queue_);
    recv_queue_ = std::move(other.recv_queue_);
    
    // 重置原对象
    other.capacity_ = 0;
    other.buffer_.clear();
    other.head_ = 0;
    other.tail_ = 0;
    other.closed_ = true;
}

template <typename T>
channel<T>& channel<T>::operator=(channel&& other) noexcept
{
    if (this != &other) {
        // 关闭当前通道
        close();
        
        // 获取其他通道的资源
        std::lock_guard<std::mutex> lock(other.mutex_);
        capacity_ = other.capacity_;
        buffer_ = std::move(other.buffer_);
        head_ = other.head_;
        tail_ = other.tail_;
        closed_ = other.closed_.load(std::memory_order_acquire);
        send_queue_ = std::move(other.send_queue_);
        recv_queue_ = std::move(other.recv_queue_);
        
        // 重置原对象
        other.capacity_ = 0;
        other.buffer_.clear();
        other.head_ = 0;
        other.tail_ = 0;
        other.closed_ = true;
    }
    return *this;
}

template <typename T>
channel<T>::~channel()
{
    close();
    
    // 唤醒所有等待的协程
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 唤醒发送者
    while (!send_queue_.empty()) {
        auto sender = send_queue_.front();
        send_queue_.pop();
        sender.handle.resume();
    }
    
    // 唤醒接收者
    while (!recv_queue_.empty()) {
        auto receiver = recv_queue_.front();
        recv_queue_.pop();
        if (receiver.result_ptr) {
            *receiver.result_ptr = std::nullopt;
        }
        receiver.handle.resume();
    }
}

template <typename T>
bool channel<T>::try_send(const T& value)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (closed_) {
        return false;
    }
    
    // 如果有等待的接收者，直接发送
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
    
    // 如果缓冲区未满，放入缓冲区
    if (capacity_ == 0) {
        // 无缓冲通道，需要等待接收者
        return false;
    }
    
    if (buffer_.size() < capacity_) {
        buffer_.push_back(value);
        return true;
    }
    
    return false;
}

template <typename T>
bool channel<T>::try_send(T&& value)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (closed_) {
        return false;
    }
    
    // 如果有等待的接收者，直接发送
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
    
    // 如果缓冲区未满，放入缓冲区
    if (capacity_ == 0) {
        // 无缓冲通道，需要等待接收者
        return false;
    }
    
    if (buffer_.size() < capacity_) {
        buffer_.push_back(std::move(value));
        return true;
    }
    
    return false;
}

template <typename T>
void channel<T>::send(const T& value)
{
    while (true) {
        if (try_send(value)) {
            return;
        }
        
        if (closed_) {
            throw std::runtime_error("send on closed channel");
        }
        
        // 等待通知
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() {
            return is_closed() || !is_full() || !recv_queue_.empty();
        });
    }
}

template <typename T>
void channel<T>::send(T&& value)
{
    while (true) {
        if (try_send(std::move(value))) {
            return;
        }
        
        if (closed_) {
            throw std::runtime_error("send on closed channel");
        }
        
        // 等待通知
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() {
            return is_closed() || !is_full() || !recv_queue_.empty();
        });
    }
}

template <typename T>
bool channel<T>::try_recv(T& value)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 如果缓冲区有数据，直接接收
    if (!buffer_.empty()) {
        value = std::move(buffer_[head_]);
        head_ = (head_ + 1) % capacity_;
        buffer_.erase(buffer_.begin());
        
        // 唤醒等待的发送者
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
    
    // 如果有等待的发送者，直接接收
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
    
    return closed_;
}

template <typename T>
T channel<T>::recv()
{
    T value;
    while (true) {
        if (try_recv(value)) {
            return value;
        }
        
        if (closed_) {
            throw std::runtime_error("recv on closed channel");
        }
        
        // 等待通知
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() {
            return is_closed() || !is_empty() || !send_queue_.empty();
        });
    }
}

template <typename T>
void channel<T>::close()
{
    if (closed_.exchange(true)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    cv_.notify_all();
}

template <typename T>
void channel<T>::push_sender(std::coroutine_handle<> handle, const T& value, bool is_rvalue)
{
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
void channel<T>::push_receiver(std::coroutine_handle<> handle, T* value_ptr, std::optional<T>* result_ptr)
{
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
        
        if (receiver.value_ptr) {
            *receiver.value_ptr = std::move(buffer_[head_]);
        }
        if (receiver.result_ptr) {
            *receiver.result_ptr = std::move(buffer_[head_]);
        }
        
        head_ = (head_ + 1) % capacity_;
        buffer_.erase(buffer_.begin());
        
        receiver.handle.resume();
    } else {
        cv_.notify_all();
    }
}

template <typename T>
void channel<T>::notify_waiters()
{
    cv_.notify_all();
}

} // namespace tang

#endif // TANG_CHANNEL_H
