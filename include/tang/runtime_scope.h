#ifndef TANG_RUNTIME_SCOPE_H
#define TANG_RUNTIME_SCOPE_H

#include <tang/runtime.h>
#include <tang/logger.h>
#include <atomic>
#include <thread>
#include <chrono>

namespace tang {

/**
 * @brief RAII wrapper for managing runtime lifecycle in tests
 * 
 * This class provides automatic initialization and cleanup of the runtime
 * scheduler using RAII principles. It ensures proper resource management
 * and test isolation.
 */
class RuntimeScope {
private:
    static std::atomic<int> instance_count_;
    static std::atomic<bool> runtime_initialized_;
    static std::atomic<bool> runtime_running_;
    
    size_t thread_count_;
    bool has_run_;
    
public:
    /**
     * @brief Construct a new RuntimeScope object
     * 
     * @param threads Number of worker threads (0 for auto-detection)
     */
    RuntimeScope(size_t threads = 2) : thread_count_(threads), has_run_(false) {
        // Only initialize runtime if this is the first instance
        if (instance_count_++ == 0) {
            LOG_INFO(tang::logger::test, "Initializing runtime with " + std::to_string(thread_count_) + " threads");
            runtime::init(thread_count_);
            runtime_initialized_ = true;
            runtime_running_ = true;
        }
    }
    
    /**
     * @brief Destroy the RuntimeScope object
     * 
     * Automatically stops the runtime when the last instance is destroyed
     */
    ~RuntimeScope() {
        // Only stop runtime when the last instance is destroyed
        if (--instance_count_ == 0) {
            LOG_INFO(tang::logger::test, "Stopping runtime");
            runtime::stop();
            runtime_initialized_ = false;
            runtime_running_ = false;
        }
    }
    
    // Delete copy constructor and assignment operator
    RuntimeScope(const RuntimeScope&) = delete;
    RuntimeScope& operator=(const RuntimeScope&) = delete;
    
    /**
     * @brief Run the scheduler to process tasks
     * 
     * This method can only be called once per instance to prevent
     * multiple scheduler runs which could cause issues.
     */
    void run() {
        if (has_run_) {
            LOG_WARN(tang::logger::test, "RuntimeScope::run() called multiple times, ignoring subsequent calls");
            return;
        }
        
        if (!runtime_running_) {
            LOG_WARN(tang::logger::test, "Runtime is not running, cannot execute run()");
            return;
        }
        
        has_run_ = true;
        
        LOG_INFO(tang::logger::runtime, "Starting scheduler run");
        
        // Run scheduler once - coroutines will properly suspend/resume via awaiters
        runtime::run();
        
        LOG_INFO(tang::logger::runtime, "Scheduler run completed");
    }
    
    /**
     * @brief Get the current instance count
     * 
     * @return int Number of active RuntimeScope instances
     */
    static int get_instance_count() {
        return instance_count_.load();
    }
    
    /**
     * @brief Check if runtime is initialized
     * 
     * @return true if runtime is initialized
     * @return false if runtime is not initialized
     */
    static bool is_runtime_initialized() {
        return runtime_initialized_.load();
    }
    
    /**
     * @brief Check if runtime is running
     * 
     * @return true if runtime is running
     * @return false if runtime is not running
     */
    static bool is_runtime_running() {
        return runtime_running_.load();
    }
};

} // namespace tang

#endif // TANG_RUNTIME_SCOPE_H