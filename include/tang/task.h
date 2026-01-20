#ifndef TANG_TASK_H
#define TANG_TASK_H

#include <coroutine>
#include <optional>
#include <stdexcept>
#include <functional>
#include <type_traits>

namespace tang {

// 前向声明
namespace runtime {
    void schedule(std::coroutine_handle<> handle);
}

// 协程任务类
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
        
        std::suspend_always initial_suspend() noexcept {
            return {};
        }
        
        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            
            template <typename PromiseType>
            std::coroutine_handle<> await_suspend(std::coroutine_handle<PromiseType> handle) noexcept {
                auto& promise = handle.promise();
                if (promise.continuation) {
                    return promise.continuation;
                }
                return std::noop_coroutine();
            }
            
            void await_resume() noexcept {}
        };
        
        final_awaiter final_suspend() noexcept {
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
        if (handle) {
            runtime::schedule(handle);
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
        
        std::suspend_always initial_suspend() noexcept {
            return {};
        }
        
        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            
            template <typename PromiseType>
            std::coroutine_handle<> await_suspend(std::coroutine_handle<PromiseType> handle) noexcept {
                auto& promise = handle.promise();
                if (promise.continuation) {
                    return promise.continuation;
                }
                return std::noop_coroutine();
            }
            
            void await_resume() noexcept {}
        };
        
        final_awaiter final_suspend() noexcept {
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
            runtime::schedule(handle);
        }
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
    using result_type = decltype(f(std::forward<Args>(args)...));
    return go_helper<result_type>::create(std::forward<F>(f), std::forward<Args>(args)...);
}

template <typename F, typename... Args>
auto spawn(F&& f, Args&&... args) {
    using result_type = decltype(f(std::forward<Args>(args)...));
    return go_helper<result_type>::create(std::forward<F>(f), std::forward<Args>(args)...);
}

} // namespace tang

#endif // TANG_TASK_H