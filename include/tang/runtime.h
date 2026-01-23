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
#include <tang/event_loop.h>

namespace tang {
namespace runtime {

class scheduler;

extern scheduler* g_scheduler;

void init(size_t num_threads = 0);
void run();
void stop();
void schedule(std::coroutine_handle<> handle);
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
    
private:
    std::vector<std::thread> threads_;
    std::list<std::coroutine_handle<>> task_queue_;  // Use list to implement FIFO
    std::mutex queue_mutex_;
    std::atomic_bool running_;
    std::unique_ptr<event_loop> event_loop_;  ///< Event loop instance
};

} // namespace runtime
} // namespace tang

#endif // TANG_RUNTIME_H