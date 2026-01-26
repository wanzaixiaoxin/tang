#ifndef TANG_RUNTIME_H
#define TANG_RUNTIME_H

#include <coroutine>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <chrono>
#include <list>
#include <condition_variable>
#include <tang/event_loop.h>

namespace tang {
namespace runtime {

class scheduler;

extern scheduler* g_scheduler;

void init(size_t num_threads = 0);
void run();
void stop();
void schedule(std::coroutine_handle<> handle);
void task_started();
void task_completed();
void yield();
void sleep_ms(size_t ms);

class scheduler {
public:
    scheduler(size_t num_threads = 0);
    ~scheduler();
    
    void init();
    void run();
    void stop();
    
    void schedule(std::coroutine_handle<> handle);
    
    /**
     * @brief Get the event loop instance
     * 
     * @return Reference to the event loop
     */
    event_loop& get_event_loop();
    
    /**
     * @brief Wait for all tasks to complete
     */
    void wait_for_completion();
    
    /**
     * @brief Check if all tasks are completed
     */
    bool is_completed();
    
    /**
     * @brief Notify that a task has started
     */
    void task_started();
    
    /**
     * @brief Notify that a task has completed
     */
    void task_completed();
    
private:
    std::vector<std::thread> threads_;
    std::list<std::coroutine_handle<>> task_queue_;  // Use list to implement FIFO
    std::mutex queue_mutex_;
    std::atomic_bool running_;
    std::atomic_int active_tasks_{0};
    std::condition_variable completion_cv_;
    std::mutex completion_mutex_;
    std::unique_ptr<event_loop> event_loop_;  ///< Event loop instance
};

} // namespace runtime
} // namespace tang

#endif // TANG_RUNTIME_H