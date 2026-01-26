#include <tang/runtime.h>
#include <tang/logger.h>
#include <iostream>
#include <chrono>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <sstream>


namespace tang {
namespace runtime {

// Global scheduler instance
scheduler* g_scheduler = nullptr;

scheduler::scheduler(size_t num_threads) : running_(false) {
    LOG_TRACE_FUNC(tang::logger::runtime);
    LOG_TRACE(tang::logger::runtime) << "Creating scheduler with " << num_threads << " threads";
    
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) {
            num_threads = 1;  // Use single thread by default for simplicity
        }
    }
    LOG_TRACE(tang::logger::runtime) << "Final thread count: " << num_threads;
    threads_.reserve(num_threads);
    
    // Initialize event loop
    event_loop_ = std::make_unique<event_loop>();
    LOG_TRACE(tang::logger::runtime) << "Event loop initialized";
}

scheduler::~scheduler() {
    stop();
}

void scheduler::init() {
    LOG_TRACE_FUNC(tang::logger::runtime);
    
    if (running_.exchange(true)) {
        LOG_TRACE(tang::logger::runtime) << "Scheduler already running, skipping init";
        return;
    }
    
    LOG_TRACE(tang::logger::runtime) << "Starting " << threads_.capacity() << " worker threads";
    
    for (size_t i = 0; i < threads_.capacity(); ++i) {
        try {
            threads_.emplace_back([this, i]() {
                LOG_TRACE(tang::logger::runtime) << "Worker thread " << i << " started";
                
                while (running_.load()) {
                    std::coroutine_handle<> handle;
                    
                    {
                        std::lock_guard<std::mutex> lock(queue_mutex_);
                        if (!task_queue_.empty()) {
                            // Use FIFO, get task from front of queue
                            handle = task_queue_.front();
                            task_queue_.pop_front();
                            LOG_TRACE(tang::logger::runtime) << "Thread " << i << " got task from queue, queue size: " << task_queue_.size();
                        }
                    }
                    
                    if (handle) {
                        LOG_TRACE_FUNC_HANDLE(tang::logger::runtime, handle);
                        
                        // Check if coroutine is already done before resuming
                        if (handle.done()) {
                            LOG_TRACE(tang::logger::runtime) << "Thread " << i << " coroutine already done, skipping resume";
                            task_completed(); // Task already completed
                            continue;
                        }
                        
                        // Check if coroutine handle is valid
                        if (!handle.address()) {
                            LOG_TRACE(tang::logger::runtime) << "Thread " << i << " invalid coroutine handle, skipping resume";
                            task_completed(); // Invalid task, consider completed
                            continue;
                        }
                        
                        LOG_TRACE(tang::logger::runtime) << "Thread " << i << " resuming coroutine";
                        
                        try {
                            handle.resume();
                        } catch (const std::exception& e) {
                            LOG_ERROR(tang::logger::runtime) << "Thread " << i << " coroutine resume failed: " << e.what();
                            task_completed(); // Task failed, consider completed
                            continue;
                        } catch (...) {
                            LOG_ERROR(tang::logger::runtime) << "Thread " << i << " coroutine resume failed with unknown exception";
                            task_completed(); // Task failed, consider completed
                            continue;
                        }
                        
                        // Check if coroutine is done
                        if (handle.done()) {
                            LOG_TRACE(tang::logger::runtime) << "Thread " << i << " coroutine completed";
                            task_completed();
                        } else {
                            LOG_TRACE(tang::logger::runtime) << "Thread " << i << " coroutine not done, re-scheduling";
                            schedule(handle);
                            // Don't call task_started() here - the task is still active
                        }
                    } else {
                        // Short sleep when no tasks to reduce CPU usage
                        std::this_thread::sleep_for(std::chrono::microseconds(100));
                    }
                }
                
                LOG_TRACE(tang::logger::runtime) << "Worker thread " << i << " stopped";
            });
        } catch (const std::exception& e) {
            LOG_ERROR(tang::logger::runtime) << "Failed to create worker thread " << i << ": " << e.what();
            running_ = false;
            throw;
        } catch (...) {
            LOG_ERROR(tang::logger::runtime) << "Failed to create worker thread " << i << " with unknown error";
            running_ = false;
            throw;
        }
    }
    
    LOG_TRACE(tang::logger::runtime) << "All worker threads started";
}

void scheduler::run() {
    LOG_TRACE_FUNC(tang::logger::runtime);
    
    init();
    
    LOG_TRACE(tang::logger::runtime) << "Processing tasks in main thread...";
    
    size_t iteration = 0;
    size_t last_active_tasks = 0;
    size_t no_progress_count = 0;
    const size_t MAX_NO_PROGRESS_ITERATIONS = 100; // Maximum iterations with no progress
    
    // Continue running until no active tasks and queue is empty
    // Also check for progress to avoid infinite loops in edge cases
    while (active_tasks_.load() > 0 || !task_queue_.empty()) {
        iteration++;
        if (iteration % 100 == 0) {
            LOG_TRACE(tang::logger::runtime) << "Scheduler iteration " << iteration << 
                     ", active tasks: " << active_tasks_.load() << 
                     ", queue empty: " << (task_queue_.empty() ? "true" : "false");
        }
        
        std::coroutine_handle<> handle;
        
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (!task_queue_.empty()) {
                handle = task_queue_.front();
                task_queue_.pop_front();
                LOG_DEBUG(tang::logger::runtime) << "Main thread got task from queue, queue size: " << task_queue_.size();
            }
        }
        
        if (handle) {
            LOG_TRACE_FUNC_HANDLE(tang::logger::runtime, handle);
            
            // Check if coroutine is already done before resuming
            if (handle.done()) {
                LOG_TRACE(tang::logger::runtime) << "Main thread coroutine already done, skipping resume";
                task_completed(); // Task was already completed
                continue;
            }
            
            // Check if coroutine handle is valid
            if (!handle.address()) {
                LOG_TRACE(tang::logger::runtime) << "Main thread invalid coroutine handle, skipping resume";
                task_completed(); // Invalid task, consider completed
                continue;
            }
            
            LOG_DEBUG(tang::logger::runtime) << "Main thread resuming coroutine";

            try {
                handle.resume();
                LOG_TRACE(tang::logger::runtime) << "Main thread resume() returned";
            } catch (const std::exception& e) {
                LOG_ERROR(tang::logger::runtime) << "Main thread coroutine resume failed: " << e.what();
                task_completed(); // Task failed, consider completed
                continue;
            } catch (...) {
                LOG_ERROR(tang::logger::runtime) << "Main thread coroutine resume failed with unknown exception";
                task_completed(); // Task failed, consider completed
                continue;
            }

            // Check if coroutine is done
            if (handle.done()) {
                LOG_TRACE(tang::logger::runtime) << "Main thread coroutine completed";
                task_completed();
            } else {
                LOG_TRACE(tang::logger::runtime) << "Main thread coroutine not done, re-scheduling";
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
                LOG_TRACE(tang::logger::runtime) << "Scheduler detected no progress for " << MAX_NO_PROGRESS_ITERATIONS << " iterations, exiting";
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
        LOG_TRACE(tang::logger::runtime) << "Scheduler main loop completed, all tasks appear finished";
    } else {
        LOG_TRACE(tang::logger::runtime) << "Scheduler exiting with " << active_tasks_.load() << " active tasks and queue size: " << task_queue_.size();
    }
    
    // Additional extended wait to allow any final processing
    // This is crucial for ensuring coroutines that were just resumed have time to complete
    LOG_TRACE(tang::logger::runtime) << "Starting extended wait for final processing...";
    for (int i = 0; i < 20; ++i) {
        if (task_queue_.empty() && active_tasks_.load() == 0) {
            LOG_TRACE(tang::logger::runtime) << "All tasks completed during extended wait at iteration " << i;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Final check
    if (task_queue_.empty() && active_tasks_.load() == 0) {
        LOG_TRACE(tang::logger::runtime) << "Scheduler run completed - all tasks finished after " << iteration << " iterations";
    } else {
        LOG_TRACE(tang::logger::runtime) << "Scheduler run completed with pending tasks - active: " << active_tasks_.load() << ", queue: " << task_queue_.size();
    }
}

void scheduler::stop() {
    LOG_TRACE_FUNC(tang::logger::runtime);
    
    if (!running_.exchange(false)) {
        LOG_TRACE(tang::logger::runtime) << "Scheduler already stopped, skipping stop";
        return;
    }
    
    LOG_TRACE(tang::logger::runtime) << "Stopping " << threads_.size() << " worker threads";
    
    // Stop event loop
    if (event_loop_) {
        LOG_TRACE(tang::logger::runtime) << "Stopping event loop";
        event_loop_->stop();
    }
    
    // Wait for all threads to finish
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            LOG_TRACE(tang::logger::runtime) << "Joining worker thread";
            thread.join();
        }
    }
    threads_.clear();
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        LOG_TRACE(tang::logger::runtime) << "Clearing task queue, size: " << task_queue_.size();
        task_queue_.clear();
    }
    
    LOG_TRACE(tang::logger::runtime) << "Scheduler stopped successfully";
}

event_loop& scheduler::get_event_loop() {
    return *event_loop_;
}

void scheduler::schedule(std::coroutine_handle<> handle) {
    LOG_TRACE_FUNC(tang::logger::runtime);
    LOG_TRACE_FUNC_HANDLE(tang::logger::runtime, handle);
    
    // Check if coroutine handle is valid before scheduling
    if (!handle.address()) {
        LOG_TRACE(tang::logger::runtime) << "Invalid coroutine handle, not scheduling";
        return;
    }
    
    // Check if coroutine is already done before scheduling
    if (handle.done()) {
        LOG_TRACE(tang::logger::runtime) << "Coroutine already done, not scheduling";
        return;
    }
    
    std::lock_guard<std::mutex> lock(queue_mutex_);
    task_queue_.push_back(handle);
    // Only increment active tasks for newly scheduled tasks, not for re-scheduling
    // The active task count is managed in the worker threads when handling tasks
    LOG_TRACE(tang::logger::runtime) << "Task scheduled, queue size: " << task_queue_.size() << ", active tasks: " << active_tasks_.load();
}

void scheduler::wait_for_completion() {
    // Simple implementation: wait for a fixed time
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

bool scheduler::is_completed() {
    return task_queue_.empty();
}

void scheduler::task_started() {
    LOG_TRACE_FUNC(tang::logger::runtime);
    active_tasks_++;
    LOG_TRACE(tang::logger::runtime) << "Task started, active tasks: " << active_tasks_.load();
}

void scheduler::task_completed() {
    LOG_TRACE_FUNC(tang::logger::runtime);
    // Only decrement if active_tasks_ > 0 to avoid underflow
    if (active_tasks_.load() > 0) {
        active_tasks_--;
    }
    LOG_TRACE(tang::logger::runtime) << "Task completed, active tasks: " << active_tasks_.load();
}

// Global function implementations

void init(size_t num_threads) {
    LOG_TRACE_FUNC(tang::logger::runtime);
    LOG_TRACE(tang::logger::runtime) << "Initializing runtime with " << num_threads << " threads";
    
    // Always create a new scheduler instance to ensure clean state
    if (g_scheduler) {
        LOG_TRACE(tang::logger::runtime) << "Deleting existing scheduler instance";
        delete g_scheduler;
        g_scheduler = nullptr;
    }
    
    LOG_TRACE(tang::logger::runtime) << "Creating new scheduler instance";
    g_scheduler = new scheduler(num_threads);
    // Do NOT call init() here - it will be called by run()
    LOG_TRACE(tang::logger::runtime) << "Scheduler instance created";
}

void run() {
    LOG_TRACE_FUNC(tang::logger::runtime);
    
    if (!g_scheduler) {
        LOG_TRACE(tang::logger::runtime) << "No scheduler found, initializing with default threads";
        init(0);
    }
    
    LOG_TRACE(tang::logger::runtime) << "Starting scheduler run";
    g_scheduler->run();
    LOG_TRACE(tang::logger::runtime) << "Scheduler run completed";
}

void stop() {
    LOG_TRACE_FUNC(tang::logger::runtime);
    
    if (g_scheduler) {
        LOG_TRACE(tang::logger::runtime) << "Stopping and deleting scheduler";
        g_scheduler->stop();
        delete g_scheduler;
        g_scheduler = nullptr;
        LOG_TRACE(tang::logger::runtime) << "Scheduler stopped and deleted";
    } else {
        LOG_TRACE(tang::logger::runtime) << "No scheduler to stop";
    }
}

void schedule(std::coroutine_handle<> handle) {
    LOG_TRACE_FUNC(tang::logger::runtime);
    LOG_TRACE_FUNC_HANDLE(tang::logger::runtime, handle);
    
    // Check if coroutine handle is valid before scheduling
    if (!handle.address()) {
        LOG_TRACE(tang::logger::runtime) << "Invalid coroutine handle, not scheduling";
        return;
    }
    
    // Check if coroutine is already done before scheduling
    if (handle.done()) {
        LOG_TRACE(tang::logger::runtime) << "Coroutine already done, not scheduling";
        return;
    }
    
    if (!g_scheduler) {
        LOG_TRACE(tang::logger::runtime) << "No scheduler found, initializing with default threads";
        init(0);
    }
    
    LOG_TRACE(tang::logger::runtime) << "Scheduling coroutine";
    g_scheduler->schedule(handle);
}

void task_started() {
    LOG_TRACE_FUNC(tang::logger::runtime);
    
    if (!g_scheduler) {
        LOG_TRACE(tang::logger::runtime) << "No scheduler found, initializing with default threads";
        init(0);
    }
    
    LOG_TRACE(tang::logger::runtime) << "Notifying scheduler that task has started";
    g_scheduler->task_started();
}

void task_completed() {
    LOG_TRACE_FUNC(tang::logger::runtime);
    
    if (!g_scheduler) {
        LOG_TRACE(tang::logger::runtime) << "No scheduler found, initializing with default threads";
        init(0);
    }
    
    LOG_TRACE(tang::logger::runtime) << "Notifying scheduler that task has completed";
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