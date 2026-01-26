#include <tang/runtime.h>
#include <tang/logger.h>
#include <iostream>
#include <chrono>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <sstream>

// Debug logging macros - DISABLE CHANNEL DEBUG TO AVOID SPAM
#define RUNTIME_DEBUG 1
#define CHANNEL_DEBUG 1  // Enable channel debug for debugging

#if RUNTIME_DEBUG
#define DEBUG_LOG(msg) do { \
    std::stringstream ss; \
    ss << msg; \
    LOG_DEBUG(tang::logger::runtime, ss.str()); \
} while(0)
#define DEBUG_LOG_FUNC() LOG_DEBUG_FUNC(tang::logger::runtime)
#define DEBUG_LOG_HANDLE(handle) LOG_DEBUG_FUNC_WITH_HANDLE(tang::logger::runtime, handle)
#else
#define DEBUG_LOG(msg)
#define DEBUG_LOG_FUNC()
#define DEBUG_LOG_HANDLE(handle)
#endif

#if CHANNEL_DEBUG
#define CHANNEL_DEBUG_LOG(msg) do { \
    std::stringstream ss; \
    ss << msg; \
    LOG_INFO(tang::logger::channel, ss.str()); \
} while(0)
#else
#define CHANNEL_DEBUG_LOG(msg)
#endif

namespace tang {
namespace runtime {

// Global scheduler instance
scheduler* g_scheduler = nullptr;

scheduler::scheduler(size_t num_threads) : running_(false) {
    DEBUG_LOG_FUNC();
    DEBUG_LOG("Creating scheduler with " << num_threads << " threads");
    
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) {
            num_threads = 1;  // Use single thread by default for simplicity
        }
    }
    DEBUG_LOG("Final thread count: " << num_threads);
    threads_.reserve(num_threads);
    
    // Initialize event loop
    event_loop_ = std::make_unique<event_loop>();
    DEBUG_LOG("Event loop initialized");
}

scheduler::~scheduler() {
    stop();
}

void scheduler::init() {
    DEBUG_LOG_FUNC();
    
    if (running_.exchange(true)) {
        DEBUG_LOG("Scheduler already running, skipping init");
        return;
    }
    
    DEBUG_LOG("Starting " << threads_.capacity() << " worker threads");
    
    for (size_t i = 0; i < threads_.capacity(); ++i) {
        threads_.emplace_back([this, i]() {
            DEBUG_LOG("Worker thread " << i << " started");
            
            while (running_.load()) {
                std::coroutine_handle<> handle;
                
                {
                    std::lock_guard<std::mutex> lock(queue_mutex_);
                    if (!task_queue_.empty()) {
                        // Use FIFO, get task from front of queue
                        handle = task_queue_.front();
                        task_queue_.pop_front();
                        DEBUG_LOG("Thread " << i << " got task from queue, queue size: " << task_queue_.size());
                    }
                }
                
                if (handle) {
                    DEBUG_LOG_HANDLE(handle);
                    
                    // Check if coroutine is already done before resuming
                    if (handle.done()) {
                        DEBUG_LOG("Thread " << i << " coroutine already done, skipping resume");
                        task_completed(); // Task already completed
                        continue;
                    }
                    
                    // Check if coroutine handle is valid
                    if (!handle.address()) {
                        DEBUG_LOG("Thread " << i << " invalid coroutine handle, skipping resume");
                        task_completed(); // Invalid task, consider completed
                        continue;
                    }
                    
                    DEBUG_LOG("Thread " << i << " resuming coroutine");
                    
                    try {
                        handle.resume();
                    } catch (const std::exception& e) {
                        DEBUG_LOG("Thread " << i << " coroutine resume failed: " << e.what());
                        task_completed(); // Task failed, consider completed
                        continue;
                    } catch (...) {
                        DEBUG_LOG("Thread " << i << " coroutine resume failed with unknown exception");
                        task_completed(); // Task failed, consider completed
                        continue;
                    }
                    
                    // Check if coroutine is done
                    if (handle.done()) {
                        DEBUG_LOG("Thread " << i << " coroutine completed");
                        task_completed();
                    } else {
                        DEBUG_LOG("Thread " << i << " coroutine not done, re-scheduling");
                        // Re-schedule the coroutine if it's not done
                        // Don't call task_completed() since the task is still active
                        schedule(handle);
                        // Important: Also increment active tasks when re-scheduling
                        task_started();
                    }
                } else {
                    // Short sleep when no tasks to reduce CPU usage
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
            }
            
            DEBUG_LOG("Worker thread " << i << " stopped");
        });
    }
    
    DEBUG_LOG("All worker threads started");
}

void scheduler::run() {
    DEBUG_LOG_FUNC();
    
    init();
    
    DEBUG_LOG("Processing tasks in main thread...");
    
    size_t iteration = 0;
    size_t last_active_tasks = 0;
    size_t no_progress_count = 0;
    const size_t MAX_NO_PROGRESS_ITERATIONS = 100; // Maximum iterations with no progress
    
    // Continue running until no active tasks and queue is empty
    // Also check for progress to avoid infinite loops in edge cases
    while (active_tasks_.load() > 0 || !task_queue_.empty()) {
        iteration++;
        if (iteration % 100 == 0) {
            DEBUG_LOG("Scheduler iteration " << iteration << 
                     ", active tasks: " << active_tasks_.load() << 
                     ", queue empty: " << (task_queue_.empty() ? "true" : "false"));
        }
        
        std::coroutine_handle<> handle;
        
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (!task_queue_.empty()) {
                handle = task_queue_.front();
                task_queue_.pop_front();
                DEBUG_LOG("Main thread got task from queue, queue size: " << task_queue_.size());
            }
        }
        
        if (handle) {
            DEBUG_LOG_HANDLE(handle);
            
            // Check if coroutine is already done before resuming
            if (handle.done()) {
                DEBUG_LOG("Main thread coroutine already done, skipping resume");
                task_completed(); // Task was already completed
                continue;
            }
            
            // Check if coroutine handle is valid
            if (!handle.address()) {
                DEBUG_LOG("Main thread invalid coroutine handle, skipping resume");
                task_completed(); // Invalid task, consider completed
                continue;
            }
            
            DEBUG_LOG("Main thread resuming coroutine");

            try {
                handle.resume();
                DEBUG_LOG("Main thread resume() returned");
            } catch (const std::exception& e) {
                DEBUG_LOG("Main thread coroutine resume failed: " << e.what());
                task_completed(); // Task failed, consider completed
                continue;
            } catch (...) {
                DEBUG_LOG("Main thread coroutine resume failed with unknown exception");
                task_completed(); // Task failed, consider completed
                continue;
            }

            // Check if coroutine is done
            if (handle.done()) {
                DEBUG_LOG("Main thread coroutine completed");
                task_completed();
            } else {
                DEBUG_LOG("Main thread coroutine not done, re-scheduling");
                // Re-schedule the coroutine if it's not done
                // Don't call task_completed() since the task is still active
                schedule(handle);
            }
        } else {
            // Queue empty, sleep to avoid busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        // Check for progress to avoid infinite loops
        size_t current_active_tasks = active_tasks_.load();
        if (current_active_tasks == last_active_tasks && task_queue_.empty()) {
            no_progress_count++;
            if (no_progress_count >= MAX_NO_PROGRESS_ITERATIONS) {
                DEBUG_LOG("Scheduler detected no progress for " << MAX_NO_PROGRESS_ITERATIONS << " iterations, exiting");
                break;
            }
        } else {
            last_active_tasks = current_active_tasks;
            no_progress_count = 0;
        }
    }
    
    // After main loop, wait a bit more to ensure all tasks have completed
    // This handles cases where tasks were just scheduled but not yet processed
    if (task_queue_.empty() && active_tasks_.load() == 0) {
        DEBUG_LOG("Scheduler main loop completed, all tasks appear finished");
    } else {
        DEBUG_LOG("Scheduler exiting with " << active_tasks_.load() << " active tasks and queue size: " << task_queue_.size());
    }
    
    // Additional extended wait to allow any final processing
    // This is crucial for ensuring coroutines that were just resumed have time to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    DEBUG_LOG("Scheduler run completed - all tasks finished after " << iteration << " iterations");
}

void scheduler::stop() {
    DEBUG_LOG_FUNC();
    
    if (!running_.exchange(false)) {
        DEBUG_LOG("Scheduler already stopped, skipping stop");
        return;
    }
    
    DEBUG_LOG("Stopping " << threads_.size() << " worker threads");
    
    // Stop event loop
    if (event_loop_) {
        DEBUG_LOG("Stopping event loop");
        event_loop_->stop();
    }
    
    // Wait for all threads to finish
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            DEBUG_LOG("Joining worker thread");
            thread.join();
        }
    }
    threads_.clear();
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        DEBUG_LOG("Clearing task queue, size: " << task_queue_.size());
        task_queue_.clear();
    }
    
    DEBUG_LOG("Scheduler stopped successfully");
}

event_loop& scheduler::get_event_loop() {
    return *event_loop_;
}

void scheduler::schedule(std::coroutine_handle<> handle) {
    DEBUG_LOG_FUNC();
    DEBUG_LOG_HANDLE(handle);
    
    // Check if coroutine handle is valid before scheduling
    if (!handle.address()) {
        DEBUG_LOG("Invalid coroutine handle, not scheduling");
        return;
    }
    
    // Check if coroutine is already done before scheduling
    if (handle.done()) {
        DEBUG_LOG("Coroutine already done, not scheduling");
        return;
    }
    
    std::lock_guard<std::mutex> lock(queue_mutex_);
    task_queue_.push_back(handle);
    // Only increment active tasks for newly scheduled tasks, not for re-scheduling
    // The active task count is managed in the worker threads when handling tasks
    DEBUG_LOG("Task scheduled, queue size: " << task_queue_.size() << ", active tasks: " << active_tasks_.load());
}

void scheduler::wait_for_completion() {
    // Simple implementation: wait for a fixed time
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

bool scheduler::is_completed() {
    return task_queue_.empty();
}

void scheduler::task_started() {
    DEBUG_LOG_FUNC();
    active_tasks_++;
    DEBUG_LOG("Task started, active tasks: " << active_tasks_.load());
}

void scheduler::task_completed() {
    DEBUG_LOG_FUNC();
    // Only decrement if active_tasks_ > 0 to avoid underflow
    if (active_tasks_.load() > 0) {
        active_tasks_--;
    }
    DEBUG_LOG("Task completed, active tasks: " << active_tasks_.load());
}

// Global function implementations

void init(size_t num_threads) {
    DEBUG_LOG_FUNC();
    DEBUG_LOG("Initializing runtime with " << num_threads << " threads");
    
    // Always create a new scheduler instance to ensure clean state
    if (g_scheduler) {
        DEBUG_LOG("Deleting existing scheduler instance");
        delete g_scheduler;
        g_scheduler = nullptr;
    }
    
    DEBUG_LOG("Creating new scheduler instance");
    g_scheduler = new scheduler(num_threads);
    // Do NOT call init() here - it will be called by run()
    DEBUG_LOG("Scheduler instance created");
}

void run() {
    LOG_DEBUG_FUNC(tang::logger::runtime);
    
    if (!g_scheduler) {
        DEBUG_LOG("No scheduler found, initializing with default threads");
        init(0);
    }
    
    DEBUG_LOG("Starting scheduler run");
    g_scheduler->run();
    DEBUG_LOG("Scheduler run completed");
}

void stop() {
    DEBUG_LOG_FUNC();
    
    if (g_scheduler) {
        DEBUG_LOG("Stopping and deleting scheduler");
        g_scheduler->stop();
        delete g_scheduler;
        g_scheduler = nullptr;
        DEBUG_LOG("Scheduler stopped and deleted");
    } else {
        DEBUG_LOG("No scheduler to stop");
    }
}

void schedule(std::coroutine_handle<> handle) {
    DEBUG_LOG_FUNC();
    DEBUG_LOG_HANDLE(handle);
    
    // Check if coroutine handle is valid before scheduling
    if (!handle.address()) {
        DEBUG_LOG("Invalid coroutine handle, not scheduling");
        return;
    }
    
    // Check if coroutine is already done before scheduling
    if (handle.done()) {
        DEBUG_LOG("Coroutine already done, not scheduling");
        return;
    }
    
    if (!g_scheduler) {
        DEBUG_LOG("No scheduler found, initializing with default threads");
        init(0);
    }
    
    DEBUG_LOG("Scheduling coroutine");
    g_scheduler->schedule(handle);
}

void task_started() {
    DEBUG_LOG_FUNC();
    
    if (!g_scheduler) {
        DEBUG_LOG("No scheduler found, initializing with default threads");
        init(0);
    }
    
    DEBUG_LOG("Notifying scheduler that task has started");
    g_scheduler->task_started();
}

void task_completed() {
    DEBUG_LOG_FUNC();
    
    if (!g_scheduler) {
        DEBUG_LOG("No scheduler found, initializing with default threads");
        init(0);
    }
    
    DEBUG_LOG("Notifying scheduler that task has completed");
    g_scheduler->task_completed();
}

void yield() {
    std::this_thread::yield();
    std::this_thread::sleep_for(std::chrono::microseconds(1));
}

void sleep_ms(size_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

} // namespace runtime
} // namespace tang