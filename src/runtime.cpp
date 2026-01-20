#include <tang/runtime.h>
#include <iostream>
#include <chrono>

namespace tang {
namespace runtime {

// 全局调度器实例
scheduler* g_scheduler = nullptr;

scheduler::scheduler(size_t num_threads) : running_(false) {
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) {
            num_threads = 4;
        }
    }
    threads_.reserve(num_threads);
}

scheduler::~scheduler() {
    stop();
}

void scheduler::init() {
    if (running_.exchange(true)) {
        return;
    }
    
    for (size_t i = 0; i < threads_.capacity(); ++i) {
        threads_.emplace_back([this]() {
            while (running_.load()) {
                std::coroutine_handle<> handle;
                
                {
                    std::lock_guard<std::mutex> lock(queue_mutex_);
                    if (!task_queue_.empty()) {
                        handle = task_queue_.back();
                        task_queue_.pop_back();
                    }
                }
                
                if (handle) {
                    handle.resume();
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        });
    }
}

void scheduler::run() {
    init();
    std::cin.get();
    stop();
}

void scheduler::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    threads_.clear();
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        task_queue_.clear();
    }
}

void scheduler::schedule(std::coroutine_handle<> handle) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    task_queue_.push_back(handle);
}

// 全局函数实现

void init(size_t num_threads) {
    if (!g_scheduler) {
        g_scheduler = new scheduler(num_threads);
        g_scheduler->init();
    }
}

void run() {
    if (!g_scheduler) {
        init(0);
    }
    g_scheduler->run();
}

void stop() {
    if (g_scheduler) {
        g_scheduler->stop();
        delete g_scheduler;
        g_scheduler = nullptr;
    }
}

void schedule(std::coroutine_handle<> handle) {
    if (!g_scheduler) {
        init(0);
    }
    g_scheduler->schedule(handle);
}

void yield() {
    std::this_thread::yield();
}

void sleep_ms(size_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

} // namespace runtime
} // namespace tang