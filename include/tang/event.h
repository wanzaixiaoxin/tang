#ifndef TANG_EVENT_H
#define TANG_EVENT_H

#include <cstdint>

namespace tang {

enum class event_type {
    read = 1,
    write = 2,
    error = 4,
    timeout = 8
};

inline event_type operator|(event_type lhs, event_type rhs) {
    return static_cast<event_type>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

inline event_type operator&(event_type lhs, event_type rhs) {
    return static_cast<event_type>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}

inline event_type& operator|=(event_type& lhs, event_type rhs) {
    lhs = lhs | rhs;
    return lhs;
}

inline event_type& operator&=(event_type& lhs, event_type rhs) {
    lhs = lhs & rhs;
    return lhs;
}

inline event_type operator~(event_type e) {
    return static_cast<event_type>(~static_cast<uint32_t>(e));
}

struct event {
    void* source;
    uint32_t events;
};

} // namespace tang

#endif // TANG_EVENT_H
