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

// 通道类
template <typename T>
class channel : public channel_base {
private:
    size_t capacity_;
    std::vector<T> buffer_;
    mutable std::mutex mutex_;
    std::atomic<bool> closed_{false};
    
public:
    // 构造函数
    explicit channel(size_t capacity = 0) : capacity_(capacity) {
        if (capacity_ > 0) {
            buffer_.reserve(capacity_);
        }
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
    
    // 发送操作
    bool try_send(const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (closed_.load()) {
            return false;
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
        
        if (closed_.load()) {
            return false;
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
    
    // 发送操作符
    bool operator<<(const T& value) {
        return try_send(value);
    }
    
    bool operator<<(T&& value) {
        return try_send(std::move(value));
    }
    
    // 接收操作
    bool try_recv(T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (buffer_.empty()) {
            return false;
        }
        
        value = std::move(buffer_.back());
        buffer_.pop_back();
        return true;
    }
    
    // 接收操作符
    bool operator>>(T& value) {
        return try_recv(value);
    }
    
    // 关闭通道
    void close() {
        closed_.store(true);
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