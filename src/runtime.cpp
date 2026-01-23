#include <tang/runtime.h>
#include <iostream>
#include <chrono>
#include <condition_variable>
#include <atomic>

namespace tang {
namespace runtime {

// Global scheduler instance
scheduler* g_scheduler = nullptr;

scheduler::scheduler(size_t num_threads) : running_(false) {
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) {
            num_threads = 4;
        }
    }
    threads_.reserve(num_threads);
    
    // Initialize event loop
    event_loop_ = std::make_unique<event_loop>();
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
                        // Use FIFO, get task from front of queue
                        handle = task_queue_.front();
                        task_queue_.pop_front();
                    }
                }
                
                if (handle) {
                    task_started();
                    handle.resume();
                    
                    // Check if coroutine is done
                    if (handle.done()) {
                        // Do NOT destroy the coroutine here - task destructor will handle it
                        task_completed();
                    } else {
                        // Re-schedule the coroutine if it's not done
                        schedule(handle);
                        task_completed(); // Count this execution as completed
                    }
                } else {
                    // Short sleep when no tasks to reduce CPU usage
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
            }
        });
    }
}

void scheduler::run() {
    init();
    
    // Wait for all tasks to complete with timeout
    std::unique_lock<std::mutex> lock(completion_mutex_);
    if (!completion_cv_.wait_for(lock, std::chrono::seconds(5), [this]() {
        return active_tasks_.load() == 0 && task_queue_.empty();
    })) {
        std::cerr << "Warning: Tasks did not complete within timeout" << std::endl;
    }
    
    stop();
}

void scheduler::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    
    // Stop event loop
    if (event_loop_) {
        event_loop_->stop();
    }
    
    // Wait for all threads to finish
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

event_loop& scheduler::get_event_loop() {
    return *event_loop_;
}

void scheduler::schedule(std::coroutine_handle<> handle) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    task_queue_.push_back(handle);
}

void scheduler::wait_for_completion() {
    std::unique_lock<std::mutex> lock(completion_mutex_);
    completion_cv_.wait(lock, [this]() {
        return active_tasks_.load() == 0 && task_queue_.empty();
    });
}

bool scheduler::is_completed() {
    return active_tasks_.load() == 0 && task_queue_.empty();
}

void scheduler::task_started() {
    active_tasks_++;
}

void scheduler::task_completed() {
    active_tasks_--;
    if (active_tasks_.load() == 0) {
        std::lock_guard<std::mutex> lock(completion_mutex_);
        completion_cv_.notify_all();
    }
}

// Global function implementations

void init(size_t num_threads) {
    if (!g_scheduler) {
        g_scheduler = new scheduler(num_threads);
        // Do NOT call init() here - it will be called by run()
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
    // 让出CPU时间片，但不调度空句柄
    std::this_thread::sleep_for(std::chrono::microseconds(1));
}

void sleep_ms(size_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

} // namespace runtime
} // namespace tang