#ifndef TANG_EVENT_SOURCE_H
#define TANG_EVENT_SOURCE_H

#include <tang/event.h>

namespace tang {

class event_source {
public:
    virtual ~event_source() = default;
    
    virtual void* get_handle() const = 0;
    
    virtual void on_event(uint32_t events) = 0;
};

} // namespace tang

#endif // TANG_EVENT_SOURCE_H
