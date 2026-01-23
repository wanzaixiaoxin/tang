#ifndef TANG_CHANNEL_H
#define TANG_CHANNEL_H

#include <vector>
#include <mutex>
#include <atomic>
#include <utility>  // for std::move support
#include <coroutine>
#include <list>
#include <memory>
#include <thread>
#include <chrono>
#include <sstream>
#include <functional>
#include "tang/logger.h"

namespace tang {

// Forward declaration of runtime functions
namespace runtime {
    void schedule(std::coroutine_handle<> handle);
}

// Coroutine waiter base class
class coro_waiter {
public:
    virtual ~coro_waiter() = default;
    virtual void resume() = 0;
    virtual void resume_with_result(bool success) {
        // Default implementation ignores result
        resume();
    }
};

// Channel base class
class channel_base {
public:
    virtual ~channel_base() = default;
    virtual bool is_closed() const = 0;
    virtual bool is_empty() const = 0;
    virtual bool is_full() const = 0;
};

// Send waiter
template <typename T>
class send_waiter : public coro_waiter {
public:
    send_waiter(std::coroutine_handle<> handle, T&& value)
        : handle_(handle), value_(std::move(value)) {}

    void resume() override {
        ::tang::runtime::schedule(handle_);
    }

    T& get_value() {
        return value_;
    }

private:
    std::coroutine_handle<> handle_;
    T value_;
};

// Receive waiter
template <typename T>
class recv_waiter : public coro_waiter {
public:
    recv_waiter(std::coroutine_handle<> handle, T* value_ptr, std::function<void(bool)> result_callback)
        : handle_(handle), value_ptr_(value_ptr), result_callback_(std::move(result_callback)) {}

    void resume() override {
        if (result_callback_) {
            result_callback_(true); // Assume success for normal resume
        }
        ::tang::runtime::schedule(handle_);
    }

    void resume_with_result(bool success) {
        if (result_callback_) {
            result_callback_(success);
        }
        ::tang::runtime::schedule(handle_);
    }

    T* get_value_ptr() {
        return value_ptr_;
    }

private:
    std::coroutine_handle<> handle_;
    T* value_ptr_;
    std::function<void(bool)> result_callback_;
};

// Channel class forward declaration
template <typename T>
class channel;

// Channel send awaiter
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

// Channel receive awaiter
template <typename T>
class channel_recv_awaiter {
public:
    channel_recv_awaiter(channel<T>& ch, T& value)
        : ch_(ch), value_(value), result_(false) {}

    bool await_ready() {
        return ch_.try_recv(value_);
    }

    void await_suspend(std::coroutine_handle<> handle) {
        ch_.register_recv_waiter(handle, &value_, [this](bool success) {
            this->set_result(success);
        });
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

// Channel class
template <typename T>
class channel : public channel_base {
private:
    size_t capacity_;
    std::list<T> buffer_;  // Use list for FIFO
    mutable std::mutex mutex_;
    std::atomic<bool> closed_{false};

    // Waiter lists
    std::list<std::unique_ptr<send_waiter<T>>> send_waiters_;
    std::list<std::unique_ptr<recv_waiter<T>>> recv_waiters_;

public:
    // Constructor
    explicit channel(size_t capacity = 0) : capacity_(capacity) {
    }

    // Disable copy and move
    channel(const channel&) = delete;
    channel& operator=(const channel&) = delete;
    channel(channel&&) = delete;
    channel& operator=(channel&&) = delete;

    // Destructor
    ~channel() {
        close();
    }
    
    // Try send
    bool try_send(const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::stringstream debug_msg;
        debug_msg << "try_send called, buffer size: " << buffer_.size() 
                  << ", recv_waiters: " << recv_waiters_.size() 
                  << ", send_waiters: " << send_waiters_.size();
        LOG_DEBUG(tang::logger::channel, debug_msg.str());

        if (closed_) {
            LOG_DEBUG(tang::logger::channel, "Channel closed, send failed");
            return false;
        }

        // If there are receive waiters, send directly
        if (!recv_waiters_.empty()) {
            LOG_DEBUG(tang::logger::channel, "Sending directly to receiver waiter");
            auto waiter = std::move(recv_waiters_.front());
            recv_waiters_.pop_front();
            *(waiter->get_value_ptr()) = value;
            waiter->resume();
            return true;
        }

        // Otherwise check if buffer is full
        if (capacity_ == 0 || buffer_.size() >= capacity_) {
            LOG_DEBUG(tang::logger::channel, "Buffer full, send failed");
            return false;
        }

        LOG_DEBUG(tang::logger::channel, "Adding to buffer");
        buffer_.push_back(value);
        return true;
    }

    bool try_send(T&& value) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (closed_) {
            return false;
        }

        // If there are receive waiters, send directly
        if (!recv_waiters_.empty()) {
            auto waiter = std::move(recv_waiters_.front());
            recv_waiters_.pop_front();
            *(waiter->get_value_ptr()) = std::move(value);
            waiter->resume();
            return true;
        }

        // Otherwise check if buffer is full
        if (capacity_ == 0 || buffer_.size() >= capacity_) {
            return false;
        }

        buffer_.push_back(std::move(value));
        return true;
    }

    // Send operator - blocking send
    channel& operator<<(T&& value) {
        LOG_DEBUG(tang::logger::channel, "Sending value");
        while (!try_send(std::move(value))) {
            // Simple busy wait, should suspend coroutine in practice
            LOG_DEBUG(tang::logger::channel, "Send blocked, waiting...");
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        LOG_DEBUG(tang::logger::channel, "Send completed");
        return *this;
    }

    channel& operator<<(const T& value) {
        LOG_DEBUG(tang::logger::channel, "Sending value");
        while (!try_send(value)) {
            // Simple busy wait, should suspend coroutine in practice
            LOG_DEBUG(tang::logger::channel, "Send blocked, waiting...");
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        LOG_DEBUG(tang::logger::channel, "Send completed");
        return *this;
    }
    
    // Try receive
    bool try_recv(T& value) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (closed_ && buffer_.empty()) {
            return false;
        }

        // If there is data, receive directly
        if (!buffer_.empty()) {
            value = std::move(buffer_.front());
            buffer_.pop_front();

            // Wake up one send waiter
            if (!send_waiters_.empty()) {
                auto waiter = std::move(send_waiters_.front());
                send_waiters_.pop_front();
                buffer_.push_back(std::move(waiter->get_value()));
                waiter->resume();
            }

            return true;
        }

        // If there are send waiters, receive directly
        if (!send_waiters_.empty()) {
            auto waiter = std::move(send_waiters_.front());
            send_waiters_.pop_front();
            value = std::move(waiter->get_value());
            waiter->resume();
            return true;
        }

        return false;
    }

    // Send operation returning awaiter
    auto send(T value) -> channel_send_awaiter<T> {
        return channel_send_awaiter<T>(*this, std::move(value));
    }

    // Receive operation returning awaiter
    auto recv(T& value) -> channel_recv_awaiter<T> {
        return channel_recv_awaiter<T>(*this, value);
    }

    // Receive operator - blocking receive
    bool operator>>(T& value) {
        LOG_DEBUG(tang::logger::channel, "Attempting to receive value");
        while (!try_recv(value)) {
            // Simple busy wait, should suspend coroutine in practice
            LOG_DEBUG(tang::logger::channel, "Receive blocked, waiting...");
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        LOG_DEBUG(tang::logger::channel, "Receive completed");
        return true;
    }

    // Register send waiter
    void register_send_waiter(std::coroutine_handle<> handle, T&& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        send_waiters_.emplace_back(std::make_unique<send_waiter<T>>(handle, std::move(value)));
    }

    // Register receive waiter
    void register_recv_waiter(std::coroutine_handle<> handle, T* value_ptr, std::function<void(bool)> result_callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        recv_waiters_.emplace_back(std::make_unique<recv_waiter<T>>(handle, value_ptr, std::move(result_callback)));
    }
    
    // Close channel
    void close() {
        bool was_closed = closed_.exchange(true);
        if (was_closed) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);

        // Wake up all waiters
        for (auto& waiter : send_waiters_) {
            waiter->resume();
        }
        send_waiters_.clear();

        for (auto& waiter : recv_waiters_) {
            waiter->resume_with_result(false); // Channel closed, receive fails
        }
        recv_waiters_.clear();
    }

    // Check channel status
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