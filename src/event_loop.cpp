#include <tang/event_loop.h>
#include <vector>
#include <thread>
#include <unordered_map>
#include <functional>
#include <chrono>

namespace tang {

event_loop::event_loop() : poller_(event_poller::create()), running_(false), next_timer_id_(1) {
}

event_loop::~event_loop() {
    stop();
}

void event_loop::run() {
    if (running_.exchange(true)) {
        return;
    }
    
    std::vector<event> events;
    
    while (running_.load()) {
        auto timeout = handle_timers();
        
        events.clear();
        if (poller_) {
            int n = poller_->wait(events, timeout);
            if (n > 0) {
                for (const auto& ev : events) {
                    if (ev.source) {
                        auto* source = static_cast<event_source*>(ev.source);
                        source->on_event(ev.events);
                    }
                }
            } else if (n == 0) {
                continue;
            } else {
                continue;
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void event_loop::stop() {
    running_.store(false);
}

bool event_loop::register_event(event_source* source, uint32_t events) {
    if (poller_) {
        return poller_->register_event(source, events);
    }
    return false;
}

bool event_loop::modify_event(event_source* source, uint32_t events) {
    if (poller_) {
        return poller_->modify_event(source, events);
    }
    return false;
}

bool event_loop::delete_event(event_source* source) {
    if (poller_) {
        return poller_->delete_event(source);
    }
    return false;
}

timer_handle event_loop::add_timer(std::chrono::milliseconds delay, timer_callback cb) {
    auto now = std::chrono::steady_clock::now();
    auto expire_time = now + delay;
    
    timer_handle handle = next_timer_id_++;
    timers_[handle] = std::make_pair(expire_time, std::move(cb));
    
    return handle;
}

bool event_loop::cancel_timer(timer_handle handle) {
    return timers_.erase(handle) > 0;
}

bool event_loop::is_running() const {
    return running_.load();
}

std::chrono::milliseconds event_loop::handle_timers() {
    auto now = std::chrono::steady_clock::now();
    std::chrono::milliseconds next_timeout = std::chrono::milliseconds::max();
    
    for (auto it = timers_.begin(); it != timers_.end();) {
        auto handle = it->first;
        auto expire_time = it->second.first;
        auto callback = it->second.second;
        
        if (expire_time <= now) {
            callback();
            it = timers_.erase(it);
        } else {
            auto timeout = std::chrono::duration_cast<std::chrono::milliseconds>(expire_time - now);
            if (timeout < next_timeout) {
                next_timeout = timeout;
            }
            ++it;
        }
    }
    
    return next_timeout;
}

} // namespace tang