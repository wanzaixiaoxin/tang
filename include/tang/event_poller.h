#ifndef TANG_EVENT_POLLER_H
#define TANG_EVENT_POLLER_H

#include <tang/event.h>
#include <tang/event_source.h>
#include <vector>
#include <chrono>
#include <memory>

namespace tang {

class event_poller {
public:
    virtual ~event_poller() = default;
    
    virtual int wait(std::vector<event>& events, std::chrono::milliseconds timeout) = 0;
    
    virtual bool register_event(event_source* source, uint32_t events) = 0;
    
    virtual bool modify_event(event_source* source, uint32_t events) = 0;
    
    virtual bool delete_event(event_source* source) = 0;
    
    static std::unique_ptr<event_poller> create();
};

} // namespace tang

#endif // TANG_EVENT_POLLER_H
