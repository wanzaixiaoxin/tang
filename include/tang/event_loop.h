#ifndef TANG_EVENT_LOOP_H
#define TANG_EVENT_LOOP_H

#include <tang/event_poller.h>
#include <chrono>
#include <functional>
#include <unordered_map>
#include <atomic>

namespace tang {

typedef uint64_t timer_handle;
typedef std::function<void()> timer_callback;

class event_loop {
public:
    event_loop();
    ~event_loop();
    
    void run();
    void stop();
    
    bool register_event(event_source* source, uint32_t events);
    bool modify_event(event_source* source, uint32_t events);
    bool delete_event(event_source* source);
    
    timer_handle add_timer(std::chrono::milliseconds delay, timer_callback cb);
    bool cancel_timer(timer_handle handle);
    
    bool is_running() const;
    
private:
    std::chrono::milliseconds handle_timers();
    
    std::unique_ptr<event_poller> poller_;
    std::atomic<bool> running_;
    
    uint64_t next_timer_id_;
    std::unordered_map<timer_handle, std::pair<std::chrono::steady_clock::time_point, timer_callback>> timers_;
};

} // namespace tang

#endif // TANG_EVENT_LOOP_H
