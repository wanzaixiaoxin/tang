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

// Select case type
enum class select_case_type {
    recv,    // receive operation
    send,    // send operation
    default_case  // default operation
};

// Select case base class
class select_case {
public:
    virtual ~select_case() = default;
    
    // Get case type
    virtual select_case_type type() const = 0;

    // Try execute case
    virtual bool try_execute() = 0;

    // Try check if executable
    virtual bool try_ready() = 0;

    // Register waiter
    virtual void register_waiter(std::coroutine_handle<> handle) = 0;

    // Unregister waiter
    virtual void unregister_waiter() = 0;

    // Set execution callback
    void set_callback(std::function<void()> callback) {
        callback_ = std::move(callback);
    }

    // Execute callback
    void execute_callback() {
        if (callback_) {
            callback_();
        }
    }

    // Set associated select_awaiter
    void set_awaiter(void* awaiter) {
        awaiter_ = awaiter;
    }

    // Get associated select_awaiter
    void* get_awaiter() const {
        return awaiter_;
    }
    
protected:
    std::function<void()> callback_;
    void* awaiter_ = nullptr; // associated select_awaiter
    std::coroutine_handle<> handle_; // waiting coroutine handle
};

// Receive case
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
        // Only check if executable, do not execute actual operation
        return !ch_.is_empty() || ch_.is_closed();
    }

    void register_waiter(std::coroutine_handle<> handle) override {
        handle_ = handle;
        // Register to channel's receive waiter list
        // Note: channel needs to support select waiter registration
    }

    void unregister_waiter() override {
        handle_ = nullptr;
        // Remove from channel's receive waiter list
    }
    
private:
    channel<T>& ch_;
    T* value_ptr_;
};

// Send case
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
        // Only check if executable, do not execute actual operation
        return !ch_.is_full() || ch_.is_closed();
    }

    void register_waiter(std::coroutine_handle<> handle) override {
        handle_ = handle;
        // Register to channel's send waiter list
        // Note: channel needs to support select waiter registration
    }

    void unregister_waiter() override {
        handle_ = nullptr;
        // Remove from channel's send waiter list
    }
    
private:
    channel<T>& ch_;
    T value_;
};

// select_awaiter class
class select_awaiter {
public:
    select_awaiter(std::vector<std::unique_ptr<select_case>> cases)
        : cases_(std::move(cases)) {
        // Set associated awaiter for each case
        for (auto& case_ptr : cases_) {
            case_ptr->set_awaiter(this);
        }
    }

    bool await_ready() {
        // Shuffle randomly for fairness
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(cases_.begin(), cases_.end(), g);

        // Check if any case can execute immediately
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

        // Register waiters for all cases
        for (auto& case_ptr : cases_) {
            case_ptr->register_waiter(handle);
        }

        // Check again if any case can execute immediately (prevent race condition)
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(cases_.begin(), cases_.end(), g);

        for (auto& case_ptr : cases_) {
            if (case_ptr->try_ready()) {
                selected_case_ = case_ptr.get();
                selected_case_->try_execute();

                // Unregister waiters for all cases
                for (auto& cp : cases_) {
                    cp->unregister_waiter();
                }

                handle.resume();
                return;
            }
        }
    }

    void await_resume() noexcept {
        // Unregister waiters for all cases
        for (auto& case_ptr : cases_) {
            case_ptr->unregister_waiter();
        }

        // Execution of selected case callback is already done in try_execute
    }

    // Wake up waiting coroutine
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

// Coroutine await select
template <typename... Cases>
auto co_select(Cases&&... cases) {
    // Convert to unique_ptr array
    std::vector<std::unique_ptr<select_case>> case_ptrs;
    case_ptrs.reserve(sizeof...(Cases));

    // Collect all cases
    ([&case_ptrs](auto& case_obj) {
        case_ptrs.push_back(std::make_unique<std::remove_reference_t<decltype(case_obj)>>(
            std::forward<decltype(case_obj)>(case_obj)));
    }(cases), ...);

    return select_awaiter(std::move(case_ptrs));
}

// Simplified select function, execute select logic directly
template <typename... Cases>
void select(Cases&&... cases) {
    // Collect all cases into vector
    std::vector<std::unique_ptr<select_case>> case_ptrs;
    case_ptrs.reserve(sizeof...(Cases));

    // Collect all cases
    ([&case_ptrs](auto& case_obj) {
        case_ptrs.push_back(std::make_unique<std::remove_reference_t<decltype(case_obj)>>(
            std::forward<decltype(case_obj)>(case_obj)));
    }(cases), ...);

    // Shuffle randomly for fairness
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(case_ptrs.begin(), case_ptrs.end(), g);

    // Try to execute case immediately
    for (auto& case_ptr : case_ptrs) {
        if (case_ptr->try_ready()) {
            case_ptr->try_execute();
            return;
        }
    }

    // If no immediately executable case, check if there is default case
    for (auto& case_ptr : case_ptrs) {
        if (case_ptr->type() == select_case_type::default_case) {
            case_ptr->try_execute();
            return;
        }
    }

    // If no default case, busy wait until a case can execute
    while (true) {
        // Shuffle randomly for fairness
        std::shuffle(case_ptrs.begin(), case_ptrs.end(), g);

        for (auto& case_ptr : case_ptrs) {
            if (case_ptr->try_ready()) {
                case_ptr->try_execute();
                return;
            }
        }

        // Brief sleep to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
}

// Helper functions: create receive case
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

// Helper functions: create send case
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

// Default case class
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
        return true; // default case is always executable
    }

    void register_waiter(std::coroutine_handle<> handle) override {
        (void)handle; // default case does not need to wait
    }

    void unregister_waiter() override {
        // default case does not need to unregister
    }

private:
    std::function<void()> callback_;
};

// Default case helper function
auto default_case(std::function<void()> callback) {
    return default_case_class(std::move(callback));
}

} // namespace tang

#endif // TANG_SELECT_H