#include <tang/runtime.h>
#include <iostream>
#include <chrono>
#include <condition_variable>

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
                        // 使用FIFO，从队列前端获取任务
                        handle = task_queue_.front();
                        task_queue_.pop_front();
                    }
                }
                
                if (handle) {
                    handle.resume();
                } else {
                    // 无任务时短暂睡眠，减少CPU占用
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
            }
        });
    }
}

void scheduler::run() {
    init();
    
    // 运行一段时间，让任务有机会完成
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    stop();
}

void scheduler::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    
    // 等待所有线程结束
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
    // 让出后重新调度
    schedule(std::coroutine_handle<>::from_address(nullptr));
}

void sleep_ms(size_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

} // namespace runtime
} // namespace tang