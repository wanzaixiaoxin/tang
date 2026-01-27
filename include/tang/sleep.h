#ifndef TANG_SLEEP_H
#define TANG_SLEEP_H

#include <coroutine>
#include <chrono>
#include <thread>
#include <future>
#include <functional>
#include "tang/task.h"

namespace tang {

namespace detail {
    // Sleep promise type for async sleep operation
    struct sleep_promise {
        std::chrono::steady_clock::time_point wake_time;
        std::coroutine_handle<> continuation;
        
        task<void> get_return_object() {
            return task<void>{std::coroutine_handle<sleep_promise>::from_promise(*this)};
        }
        
        std::suspend_always initial_suspend() { 
            return {}; 
        }
        
        std::suspend_always final_suspend() noexcept { 
            return {}; 
        }
        
        void return_void() {}
        
        void unhandled_exception() {}
    };
}

// A proper async sleep implementation that doesn't block threads
class sleep_task {
public:
    struct promise_type {
        std::chrono::steady_clock::time_point wake_time;
        std::coroutine_handle<> continuation;

        sleep_task get_return_object() {
            return sleep_task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_never initial_suspend() noexcept {
            return {};
        }

        std::suspend_always final_suspend() noexcept {
            return {};
        }

        void return_void() noexcept {}

        void unhandled_exception() noexcept {}
    };

    using handle_type = std::coroutine_handle<promise_type>;

    explicit sleep_task(handle_type h) noexcept : handle(h) {}

    sleep_task(sleep_task&& other) noexcept : handle(other.handle) {
        other.handle = nullptr;
    }

    sleep_task& operator=(sleep_task&& other) noexcept {
        if (this != &other) {
            if (handle) {
                handle.destroy();
            }
            handle = other.handle;
            other.handle = nullptr;
        }
        return *this;
    }

    sleep_task(const sleep_task&) = delete;
    sleep_task& operator=(const sleep_task&) = delete;

    ~sleep_task() {
        if (handle) {
            handle.destroy();
        }
    }

    bool await_ready() const noexcept {
        return false; // Never ready immediately
    }

    void await_suspend(std::coroutine_handle<> continuation) {
        auto& promise = handle.promise();
        promise.continuation = continuation;
        
        // Store the time when we should wake up
        auto& sleep_promise = handle.promise();
        auto now = std::chrono::steady_clock::now();
        sleep_promise.wake_time = now + sleep_duration_;
        
        // Schedule a thread to wake us up later
        std::thread([this, handle_copy = handle]() mutable {
            auto& p = handle_copy.promise();
            auto sleep_until = p.wake_time;
            
            // Busy wait with small sleeps to check if we should wake up
            while (std::chrono::steady_clock::now() < sleep_until) {
                auto remaining = sleep_until - std::chrono::steady_clock::now();
                if (remaining <= std::chrono::microseconds(0)) {
                    break;
                }
                
                // Sleep for a small amount of time to avoid busy spinning too much
                auto sleep_time = std::min(remaining, std::chrono::microseconds(100));
                std::this_thread::sleep_for(sleep_time);
            }
            
            // Resume the coroutine
            if (p.continuation) {
                runtime::schedule(p.continuation);
            }
        }).detach();
    }

    void await_resume() const noexcept {}

    static sleep_task create(std::chrono::milliseconds duration) {
        sleep_task::promise_type p{};
        auto h = std::coroutine_handle<sleep_task::promise_type>::from_promise(p);
        sleep_task t(h);
        t.sleep_duration_ = duration;
        return t;
    }

private:
    handle_type handle;
    std::chrono::milliseconds sleep_duration_{0};
};

// Non-blocking sleep function that properly suspends the coroutine
inline task<void> sleep(std::chrono::milliseconds duration) {
    // Create a temporary thread to track sleep time and schedule resume
    struct awaiter {
        std::chrono::milliseconds duration_;
        
        bool await_ready() const noexcept {
            return false; // Never ready immediately
        }
        
        void await_suspend(std::coroutine_handle<> continuation) {
            // Launch a detached thread to wake up the coroutine after the specified duration
            std::thread([continuation, this]() {
                std::this_thread::sleep_for(duration_);
                runtime::schedule(continuation);
            }).detach();
        }
        
        void await_resume() const noexcept {}
    };
    
    co_await awaiter{duration};
}

} // namespace tang

#endif // TANG_SLEEP_H