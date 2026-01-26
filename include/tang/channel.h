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
        (void)success; // Mark parameter as used to suppress warning
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

    void resume_with_result(bool success) override {
        // For send waiter, we typically just resume regardless of success
        (void)success; // Mark parameter as used to suppress warning
        ::tang::runtime::schedule(handle_);
    }

    T& get_value() {
        return value_;
    }

private:
    std::coroutine_handle<> handle_;
    T value_;
};

// Forward declaration
template <typename T>
class channel_recv_awaiter;

// Receive waiter
template <typename T>
class recv_waiter : public coro_waiter {
public:
    recv_waiter(std::coroutine_handle<> handle, T* value_ptr, bool* result_ptr, channel_recv_awaiter<T>* awaiter_ptr)
        : handle_(handle), value_ptr_(value_ptr), result_ptr_(result_ptr), awaiter_ptr_(awaiter_ptr) {
        LOG_INFO(tang::logger::channel) << "recv_waiter constructed";
    }

    void resume() override {
        LOG_INFO(tang::logger::channel) << "recv_waiter::resume called, scheduling receiver coroutine";
        // Set result flag to true if the pointer is valid
        if (result_ptr_) {
            LOG_INFO(tang::logger::channel) << "recv_waiter::resume setting result_ptr_ to true";
            *result_ptr_ = true;
        } else {
            LOG_INFO(tang::logger::channel) << "recv_waiter::resume result_ptr_ is null";
        }
        // Directly set awaiter's result_ member if available
        if (awaiter_ptr_) {
            awaiter_ptr_->set_result(true);
            LOG_INFO(tang::logger::channel) << "recv_waiter::resume set awaiter.result_ to true";
        } else {
            LOG_INFO(tang::logger::channel) << "recv_waiter::resume awaiter_ptr_ is null";
        }
        // Schedule the coroutine to run
        LOG_INFO(tang::logger::channel) << "Scheduling receiver coroutine handle: " << reinterpret_cast<size_t>(handle_.address());
        ::tang::runtime::schedule(handle_);
    }

    void resume_with_result(bool success) override {
        LOG_INFO(tang::logger::channel) << "recv_waiter::resume_with_result called, success = " << (success ? "true" : "false");
        // Set result flag if the pointer is valid
        if (result_ptr_) {
            LOG_INFO(tang::logger::channel) << "recv_waiter::resume_with_result setting result_ptr_ to " << (success ? "true" : "false");
            *result_ptr_ = success;
        } else {
            LOG_INFO(tang::logger::channel) << "recv_waiter::resume_with_result result_ptr_ is null";
        }
        // Directly set awaiter's result_ member if available
        if (awaiter_ptr_) {
            awaiter_ptr_->set_result(success);
            LOG_INFO(tang::logger::channel) << "recv_waiter::resume_with_result set awaiter.result_ to " << (success ? "true" : "false");
        } else {
            LOG_INFO(tang::logger::channel) << "recv_waiter::resume_with_result awaiter_ptr_ is null";
        }
        // Schedule the coroutine to run
         LOG_INFO(tang::logger::channel) << "Scheduling receiver coroutine handle: " << reinterpret_cast<size_t>(handle_.address());
         ::tang::runtime::schedule(handle_);
    }

    T* get_value_ptr() {
        return value_ptr_;
    }

private:
    std::coroutine_handle<> handle_;
    T* value_ptr_;
    bool* result_ptr_;
    channel_recv_awaiter<T>* awaiter_ptr_;
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
        LOG_INFO(tang::logger::channel) << "channel_send_awaiter::await_ready called";
        bool ready = ch_.try_send(value_);
        LOG_INFO(tang::logger::channel) << "channel_send_awaiter::await_ready returns: " << (ready ? "true" : "false");
        return ready;
    }

    void await_suspend(std::coroutine_handle<> handle) {
        LOG_INFO(tang::logger::channel) << "channel_send_awaiter::await_suspend called, registering sender waiter";
        ch_.register_send_waiter(handle, std::move(value_));
    }

    void await_resume() {
        LOG_INFO(tang::logger::channel) << "channel_send_awaiter::await_resume called";
    }

private:
    channel<T>& ch_;
    T value_;
};

// Channel receive awaiter
template <typename T>
class channel_recv_awaiter {
public:
    channel_recv_awaiter(channel<T>& ch, T& value)
        : ch_(ch), value_(value), result_(false) {
        LOG_INFO(tang::logger::channel) << "channel_recv_awaiter constructed, result_ address: " << reinterpret_cast<size_t>(&result_);
    }

    ~channel_recv_awaiter() {
        LOG_INFO(tang::logger::channel) << "channel_recv_awaiter destroyed, result_ = " << (result_ ? "true" : "false");
    }

    bool await_ready() {
        LOG_INFO(tang::logger::channel) << "channel_recv_awaiter::await_ready called";
        bool success = ch_.try_recv(value_);
        LOG_INFO(tang::logger::channel) << "channel_recv_awaiter::await_ready returns: " << (success ? "true" : "false");
        if (success) {
            result_ = true;
            LOG_INFO(tang::logger::channel) << "channel_recv_awaiter::result_ set to true";
        }
        return success;
    }

    void await_suspend(std::coroutine_handle<> handle) {
        LOG_INFO(tang::logger::channel) << "channel_recv_awaiter::await_suspend called, registering recv waiter";
        ch_.register_recv_waiter(handle, &value_, &result_, this);
    }

    bool await_resume() {
        std::stringstream log_ss;
        log_ss << "channel_recv_awaiter::await_resume called, result_ = " << (result_ ? "true" : "false")
               << ", channel closed: " << (ch_.is_closed() ? "true" : "false");
        
        // Add safety check to prevent potential crashes
        if (result_) {
            try {
                log_ss << ", value: ";
                if constexpr (std::is_integral_v<T>) {
                    log_ss << value_;
                } else {
                    log_ss << "(address: " << reinterpret_cast<size_t>(&value_) << ")";
                }
            } catch (const std::exception& e) {
                log_ss << ", value access failed: " << e.what();
            } catch (...) {
                log_ss << ", value access failed with unknown error";
            }
        }
        
        LOG_INFO(tang::logger::channel) << log_ss.str();
        return result_;
    }

    // Allow recv_waiter to set result_
    void set_result(bool value) {
        LOG_INFO(tang::logger::channel) << "channel_recv_awaiter::set_result called, value = " << (value ? "true" : "false");
        result_ = value;
    }

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
        
        LOG_INFO(tang::logger::channel) << "try_send called, buffer size: " << buffer_.size() 
                  << ", recv_waiters: " << recv_waiters_.size() 
                  << ", send_waiters: " << send_waiters_.size()
                  << "try_send value address: " << reinterpret_cast<size_t>(&value);

        if (closed_) {
            LOG_INFO(tang::logger::channel) << "Channel closed, send failed";
            return false;
        }

        // If there are receive waiters, send directly
        if (!recv_waiters_.empty()) {
            LOG_INFO(tang::logger::channel) << "Sending directly to receiver waiter";
            auto waiter = std::move(recv_waiters_.front());
            recv_waiters_.pop_front();
            T* value_ptr = waiter->get_value_ptr();
            LOG_INFO(tang::logger::channel) << "Assigning value to receiver at address " << reinterpret_cast<size_t>(value_ptr);
            *(value_ptr) = value;
            
            // Ensure waiter object remains valid during resume
            auto resume_func = [waiter = std::move(waiter)]() {
                waiter->resume();
            };
            resume_func();
            
            LOG_INFO(tang::logger::channel) << "Receiver resumed";
            return true;
        }

        // Otherwise check if buffer is full
        if (capacity_ == 0 || buffer_.size() >= capacity_) {
            LOG_INFO(tang::logger::channel) << "Buffer full, send failed";
            return false;
        }

        LOG_INFO(tang::logger::channel) << "Adding to buffer";
        buffer_.push_back(value);
        LOG_INFO(tang::logger::channel) << "Buffer size now: " << buffer_.size();
        return true;
    }

    bool try_send(T&& value) {
        std::lock_guard<std::mutex> lock(mutex_);

        std::stringstream debug_msg;
        debug_msg << "try_send (move) called, buffer size: " << buffer_.size() 
                  << ", recv_waiters: " << recv_waiters_.size() 
                  << ", send_waiters: " << send_waiters_.size();
        LOG_INFO(tang::logger::channel) << debug_msg.str();
        LOG_INFO(tang::logger::channel) << "try_send (move) value address: " << reinterpret_cast<size_t>(&value);

        if (closed_) {
            LOG_INFO(tang::logger::channel) << "Channel closed, send failed";
            return false;
        }

        // If there are receive waiters, send directly
        if (!recv_waiters_.empty()) {
            LOG_INFO(tang::logger::channel) << "Sending directly to receiver waiter (move)"; 
            auto waiter = std::move(recv_waiters_.front());
            recv_waiters_.pop_front();
            T* value_ptr = waiter->get_value_ptr();
            LOG_INFO(tang::logger::channel) << "Assigning move value to receiver at address " << reinterpret_cast<size_t>(value_ptr);
            *(value_ptr) = std::move(value);
            waiter->resume();
            LOG_INFO(tang::logger::channel) << "Receiver resumed (move)";
            return true;
        }

        // Otherwise check if buffer is full
        if (capacity_ == 0 || buffer_.size() >= capacity_) {
            LOG_INFO(tang::logger::channel) << "Buffer full, send failed (move)";
            return false;
        }

        LOG_INFO(tang::logger::channel) << "Adding to buffer (move)";    
        buffer_.push_back(std::move(value));
        LOG_INFO(tang::logger::channel) << "Buffer size now: " << buffer_.size();    
        return true;
    }

    // Send operator - blocking send
    channel& operator<<(T&& value) {
        LOG_DEBUG(tang::logger::channel) << "Sending value";
        while (!try_send(std::move(value))) {
            // Simple busy wait, should suspend coroutine in practice
            LOG_DEBUG(tang::logger::channel) << "Send blocked, waiting...";
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        LOG_DEBUG(tang::logger::channel) << "Send completed";
        return *this;
    }

    channel& operator<<(const T& value) {
        LOG_DEBUG(tang::logger::channel) << "Sending value";
        while (!try_send(value)) {
            // Simple busy wait, should suspend coroutine in practice
            LOG_DEBUG(tang::logger::channel) << "Send blocked, waiting...";
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        LOG_DEBUG(tang::logger::channel) << "Send completed";
        return *this;
    }
    
    // Try receive
    bool try_recv(T& value) {
        std::lock_guard<std::mutex> lock(mutex_);

        LOG_INFO(tang::logger::channel) << "try_recv called, buffer size: " << buffer_.size() 
                  << ", recv_waiters: " << recv_waiters_.size() 
                  << ", send_waiters: " << send_waiters_.size();
        LOG_INFO(tang::logger::channel) << "try_recv value address: " << reinterpret_cast<size_t>(&value);

        if (closed_ && buffer_.empty()) {
            LOG_INFO(tang::logger::channel) << "Channel closed and buffer empty, receive failed";
            return false;
        }

        // If there is data, receive directly
        if (!buffer_.empty()) {
            LOG_INFO(tang::logger::channel) << "Buffer has data, receiving directly";
            value = std::move(buffer_.front());
            buffer_.pop_front();
            // Try to log the received value for integer types
            {
                std::stringstream val_ss;
                val_ss << "Received value: ";
                if constexpr (std::is_integral_v<T>) {
                    val_ss << value;
                } else {
                    val_ss << "(address: " << reinterpret_cast<size_t>(&value) << ")";
                }
                LOG_INFO(tang::logger::channel) << val_ss.str();
            }
            LOG_INFO(tang::logger::channel) << "Buffer size now: " << buffer_.size();

            // Wake up one send waiter
            if (!send_waiters_.empty()) {
                LOG_INFO(tang::logger::channel) << "Waking up send waiter";  
                auto waiter = std::move(send_waiters_.front());
                send_waiters_.pop_front();
                buffer_.push_back(std::move(waiter->get_value()));
                LOG_INFO(tang::logger::channel) << "Added sender's value to buffer, buffer size: " << buffer_.size();
                waiter->resume();
                LOG_INFO(tang::logger::channel) << "Sender resumed"; 
            }

            return true;
        }

        // If there are send waiters, receive directly
        if (!send_waiters_.empty()) {
            LOG_INFO(tang::logger::channel) << "Receiving directly from send waiter";
            auto waiter = std::move(send_waiters_.front());
            send_waiters_.pop_front();
            value = std::move(waiter->get_value());
            {
                std::stringstream val_ss;
                val_ss << "Received value directly from sender: ";
                if constexpr (std::is_integral_v<T>) {
                    val_ss << value;
                } else {
                    val_ss << "(address: " << reinterpret_cast<size_t>(&value) << ")";
                }
                LOG_INFO(tang::logger::channel) << val_ss.str();
            }
            waiter->resume();
            LOG_INFO(tang::logger::channel) << "Sender resumed, received value";
            return true;
        }

        LOG_INFO(tang::logger::channel) << "No data and no senders, receive failed"; 
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
        LOG_DEBUG(tang::logger::channel) << "Attempting to receive value";
        while (!try_recv(value)) {
            // Simple busy wait, should suspend coroutine in practice
            LOG_DEBUG(tang::logger::channel) << "Receive blocked, waiting...";
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        LOG_DEBUG(tang::logger::channel) << "Receive completed";
        return true;
    }

    // Register send waiter
    void register_send_waiter(std::coroutine_handle<> handle, T&& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        {
            LOG_DEBUG(tang::logger::channel) << "Registering send waiter, send_waiters size: " << send_waiters_.size();
        }
        
        // If channel is already closed, immediately resume (send fails)
        if (closed_) {
            LOG_DEBUG(tang::logger::channel) << "Channel closed, immediately failing send";
            ::tang::runtime::schedule(handle);
            return;
        }
        
        send_waiters_.emplace_back(std::make_unique<send_waiter<T>>(handle, std::move(value)));
        {
            LOG_DEBUG(tang::logger::channel) << "Send waiter registered, send_waiters size: " << send_waiters_.size();
        }
    }

    // Register receive waiter
    void register_recv_waiter(std::coroutine_handle<> handle, T* value_ptr, bool* result_ptr, channel_recv_awaiter<T>* awaiter_ptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        {
            LOG_DEBUG(tang::logger::channel) << "Registering recv waiter, recv_waiters size: " << recv_waiters_.size();    
        }
        
        // If channel is already closed, immediately resume with failure
        if (closed_) {
            LOG_DEBUG(tang::logger::channel) << "Channel closed, immediately failing receive";
            *result_ptr = false;
            ::tang::runtime::schedule(handle);
            return;
        }
        
        recv_waiters_.emplace_back(std::make_unique<recv_waiter<T>>(handle, value_ptr, result_ptr, awaiter_ptr));
        {
            LOG_DEBUG(tang::logger::channel) << "Recv waiter registered, recv_waiters size: " << recv_waiters_.size();        
        }
    }
    
    // Close channel
    void close() {
        {
            LOG_DEBUG(tang::logger::channel) << "close() called, closed state before: " << closed_.load();
        }
        bool was_closed = closed_.exchange(true);
        if (was_closed) {
            LOG_DEBUG(tang::logger::channel) << "Channel already closed, skipping";
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        {
            LOG_DEBUG(tang::logger::channel) << "Waking up all waiters, send_waiters: " << send_waiters_.size() 
               << ", recv_waiters: " << recv_waiters_.size();
        }

        // Wake up all waiters
        for (auto& waiter : send_waiters_) {
            LOG_DEBUG(tang::logger::channel) << "Waking up send waiter";
            waiter->resume();
        }
        send_waiters_.clear();

        for (auto& waiter : recv_waiters_) {
            LOG_DEBUG(tang::logger::channel) << "Waking up recv waiter with result=false";
            waiter->resume_with_result(false); // Channel closed, receive fails
        }
        recv_waiters_.clear();
        LOG_DEBUG(tang::logger::channel) << "All waiters cleared";
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