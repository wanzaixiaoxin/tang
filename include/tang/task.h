#ifndef TANG_TASK_H
#define TANG_TASK_H

#include <coroutine>
#include <optional>
#include <stdexcept>
#include <functional>
#include <type_traits>
#include <chrono>
#include <thread>
#include <iostream>
#include <sstream>
#include "tang/logger.h"


namespace tang {

// Forward declaration
namespace runtime {
    void schedule(std::coroutine_handle<> handle);
    void task_started();
    void task_completed();
}

// Coroutine task class
template <typename T = void>
class task {
public:
    struct promise_type {
        std::optional<T> result;
        std::exception_ptr exception;
        std::coroutine_handle<> continuation;
        
        task get_return_object() {
            return task(std::coroutine_handle<promise_type>::from_promise(*this));
        }

        std::suspend_never initial_suspend() noexcept {
            return {};
        }
        
        std::suspend_always final_suspend() noexcept {
            LOG_TRACE(logger::task) << "final_suspend called";
            return {};
        }

        void return_value(T value) noexcept {
            result.emplace(std::move(value));
        }

        void unhandled_exception() noexcept {
            exception = std::current_exception();
        }
    };
    
    using handle_type = std::coroutine_handle<promise_type>;
    
    explicit task(handle_type h) noexcept : handle(h) {}
    
    task(task&& other) noexcept : handle(other.handle) {
        other.handle = nullptr;
    }
    
    task& operator=(task&& other) noexcept {
        if (this != &other) {
            if (handle) {
                handle.destroy();
            }
            handle = other.handle;
            other.handle = nullptr;
        }
        return *this;
    }
    
    task(const task&) = delete;
    task& operator=(const task&) = delete;
    
    ~task() {
        if (handle) {
            handle.destroy();
        }
    }
    
    bool await_ready() const noexcept {
        return !handle || handle.done();
    }
    
    void await_suspend(std::coroutine_handle<> continuation) noexcept {
        auto& promise = handle.promise();
        promise.continuation = continuation;
        runtime::schedule(handle);
    }
    
    T await_resume() {
        if (!handle) {
            throw std::logic_error("task handle is null");
        }
        
        auto& promise = handle.promise();
        if (promise.exception) {
            std::rethrow_exception(promise.exception);
        }
        
        return std::move(*promise.result);
    }
    
    void run() {
        LOG_TRACE_FUNC(logger::task);
        LOG_TRACE_FUNC_HANDLE(logger::task, handle);

        if (handle) {
            // If coroutine is already completed (may happen with suspend_never), no need to schedule
            if (handle.done()) {
                LOG_TRACE(logger::task) << "Task already done, skipping schedule";
                // Notify scheduler that task has completed
                runtime::task_completed();
                // Destroy the handle since coroutine is complete
                handle.destroy();
                handle = nullptr;
                return;
            }
            LOG_TRACE(logger::task) << "Scheduling task";
            
            // Notify scheduler that a new task has started
            runtime::task_started();
            
            runtime::schedule(handle);
            LOG_TRACE(logger::task) << "Task scheduled";
        } else {
            LOG_TRACE(logger::task) << "No handle to schedule";
        }
    }
    
private:
    handle_type handle;
};

// void 特化
template <>
class task<void> {
public:
    struct promise_type {
        std::exception_ptr exception;
        std::coroutine_handle<> continuation;

        task get_return_object() {
            return task(std::coroutine_handle<promise_type>::from_promise(*this));
        }

        std::suspend_never initial_suspend() noexcept {
            return {};
        }

        std::suspend_always final_suspend() noexcept {
            return {};
        }

        void return_void() noexcept {}

        void unhandled_exception() noexcept {
            exception = std::current_exception();
        }
    };
    
    using handle_type = std::coroutine_handle<promise_type>;
    
    explicit task(handle_type h) noexcept : handle(h) {}
    
    task(task&& other) noexcept : handle(other.handle) {
        other.handle = nullptr;
    }
    
    task& operator=(task&& other) noexcept {
        if (this != &other) {
            if (handle) {
                handle.destroy();
            }
            handle = other.handle;
            other.handle = nullptr;
        }
        return *this;
    }
    
    task(const task&) = delete;
    task& operator=(const task&) = delete;
    
    ~task() {
        if (handle) {
            handle.destroy();
        }
    }
    
    bool await_ready() const noexcept {
        return !handle || handle.done();
    }
    
    void await_suspend(std::coroutine_handle<> continuation) noexcept {
        auto& promise = handle.promise();
        promise.continuation = continuation;
        runtime::schedule(handle);
    }
    
    void await_resume() {
        if (!handle) {
            throw std::logic_error("task handle is null");
        }
        
        auto& promise = handle.promise();
        if (promise.exception) {
            std::rethrow_exception(promise.exception);
        }
    }
    
    void run() {
        if (handle) {
            // If coroutine is already completed (may happen with suspend_never), no need to schedule
            if (handle.done()) {
                // Notify scheduler that task has completed
                runtime::task_completed();
                handle.destroy();
                handle = nullptr;
                return;
            }
            runtime::schedule(handle);
        }
    }
    
    static task<void> sleep(std::chrono::milliseconds duration) {
        std::this_thread::sleep_for(duration);
        co_return;
    }
    
private:
    handle_type handle;
};

template <typename R>
struct go_helper {
    template <typename F, typename... Args>
    static task<R> create(F&& f, Args&&... args) {
        co_return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    }
};

template <>
struct go_helper<void> {
    template <typename F, typename... Args>
    static task<void> create(F&& f, Args&&... args) {
        std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
        co_return;
    }
};

template <typename F, typename... Args>
auto go(F&& f, Args&&... args) {
    LOG_TRACE_FUNC(logger::task);
    LOG_TRACE(logger::task) << "Creating task from function";
    
    using result_type = decltype(std::declval<F&>()(std::declval<Args>()...));
    auto task = go_helper<result_type>::create(std::forward<F>(f), std::forward<Args>(args)...);
    
    LOG_TRACE(logger::task) << "Running task";
    task.run(); // Schedule task immediately (task.run() will call task_started())
    
    LOG_TRACE(logger::task) << "Task created and scheduled";
    return task;
}

template <typename F, typename... Args>
auto spawn(F&& f, Args&&... args) {
    using result_type = decltype(std::declval<F&>()(std::declval<Args>()...));
    auto task = go_helper<result_type>::create(std::forward<F>(f), std::forward<Args>(args)...);
    
    task.run(); // Schedule task immediately (task.run() will call task_started())
    return task;
}

} // namespace tang

#endif // TANG_TASK_H