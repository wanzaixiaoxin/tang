#ifndef TANG_RUNTIME_H
#define TANG_RUNTIME_H

#include <coroutine>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>

namespace tang {
namespace runtime {

class scheduler;

extern scheduler* g_scheduler;

void init(size_t num_threads = 0);
void run();
void stop();
void schedule(std::coroutine_handle<> handle);

class scheduler {
public:
    scheduler(size_t num_threads = 0);
    ~scheduler();
    
    void init();
    void run();
    void stop();
    
    void schedule(std::coroutine_handle<> handle);
    
private:
    std::vector<std::thread> threads_;
    std::vector<std::coroutine_handle<>> task_queue_;
    std::mutex queue_mutex_;
    std::atomic_bool running_;
};

} // namespace runtime
} // namespace tang

#endif // TANG_RUNTIME_H