#include <tang/event_poller.h>
#include <tang/event.h>
#include <tang/event_source.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>
#include <memory>
#include <thread>

namespace tang {

class epoll_event_poller : public event_poller {
public:
    epoll_event_poller();
    ~epoll_event_poller() override;
    int wait(std::vector<event>& events, std::chrono::milliseconds timeout) override;
    bool register_event(event_source* source, uint32_t events) override;
    bool modify_event(event_source* source, uint32_t events) override;
    bool delete_event(event_source* source) override;

private:
    uint32_t to_epoll_events(uint32_t events);
    uint32_t from_epoll_events(uint32_t events);
    int epoll_fd_;
    static const int MAX_EVENTS = 1024;
    std::unordered_map<int, event_source*> fd_to_source_;
};

epoll_event_poller::epoll_event_poller() : epoll_fd_(-1) {
    epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd_ == -1) {
        // Log error or throw exception
        perror("epoll_create1 failed");
    }
}

epoll_event_poller::~epoll_event_poller() {
    if (epoll_fd_ != -1) {
        close(epoll_fd_);
        epoll_fd_ = -1;
    }
    fd_to_source_.clear();
}

int epoll_event_poller::wait(std::vector<event>& events, std::chrono::milliseconds timeout) {
    if (epoll_fd_ == -1) {
        return -1;
    }
    
    struct epoll_event ep_events[MAX_EVENTS];
    int wait_time = (timeout == std::chrono::milliseconds::max()) ? -1 : static_cast<int>(timeout.count());
    
    int nfds = epoll_wait(epoll_fd_, ep_events, MAX_EVENTS, wait_time);
    
    if (nfds == -1) {
        return -1;
    }
    
    for (int i = 0; i < nfds; ++i) {
        int fd = ep_events[i].data.fd;
        uint32_t epoll_events = ep_events[i].events;
        
        auto it = fd_to_source_.find(fd);
        if (it != fd_to_source_.end()) {
            event_source* source = it->second;
            uint32_t tang_events = from_epoll_events(epoll_events);
            
            events.push_back({source, tang_events});
        }
    }
    
    return nfds;
}

bool epoll_event_poller::register_event(event_source* source, uint32_t events) {
    if (source == nullptr || epoll_fd_ == -1) {
        return false;
    }
    
    int fd = static_cast<int>(reinterpret_cast<intptr_t>(source->get_handle()));
    if (fd == -1) {
        return false;
    }
    
    uint32_t epoll_events = to_epoll_events(events);
    
    struct epoll_event ev;
    ev.events = epoll_events;
    ev.data.fd = fd;
    
    int result = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
    if (result == -1) {
        return false;
    }
    
    fd_to_source_[fd] = source;
    
    return true;
}

bool epoll_event_poller::modify_event(event_source* source, uint32_t events) {
    if (source == nullptr || epoll_fd_ == -1) {
        return false;
    }
    
    int fd = static_cast<int>(reinterpret_cast<intptr_t>(source->get_handle()));
    if (fd == -1) {
        return false;
    }
    
    auto it = fd_to_source_.find(fd);
    if (it == fd_to_source_.end()) {
        return false;
    }
    
    uint32_t epoll_events = to_epoll_events(events);
    
    struct epoll_event ev;
    ev.events = epoll_events;
    ev.data.fd = fd;
    
    int result = epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
    if (result == -1) {
        return false;
    }
    
    return true;
}

bool epoll_event_poller::delete_event(event_source* source) {
    if (source == nullptr || epoll_fd_ == -1) {
        return false;
    }
    
    int fd = static_cast<int>(reinterpret_cast<intptr_t>(source->get_handle()));
    if (fd == -1) {
        return false;
    }
    
    auto it = fd_to_source_.find(fd);
    if (it == fd_to_source_.end()) {
        return false;
    }
    
    int result = epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    if (result == -1) {
        return false;
    }
    
    fd_to_source_.erase(it);
    
    return true;
}

uint32_t epoll_event_poller::to_epoll_events(uint32_t events) {
    uint32_t epoll_events = 0;
    
    if (events & static_cast<uint32_t>(event_type::read)) {
        epoll_events |= EPOLLIN;
    }
    
    if (events & static_cast<uint32_t>(event_type::write)) {
        epoll_events |= EPOLLOUT;
    }
    
    if (events & static_cast<uint32_t>(event_type::error)) {
        epoll_events |= EPOLLERR;
    }
    
    epoll_events |= EPOLLET;  // 边缘触发
    
    return epoll_events;
}

uint32_t epoll_event_poller::from_epoll_events(uint32_t events) {
    uint32_t tang_events = 0;
    
    if (events & EPOLLIN) {
        tang_events |= static_cast<uint32_t>(event_type::read);
    }
    
    if (events & EPOLLOUT) {
        tang_events |= static_cast<uint32_t>(event_type::write);
    }
    
    if (events & EPOLLERR) {
        tang_events |= static_cast<uint32_t>(event_type::error);
    }
    
    return tang_events;
}

std::unique_ptr<event_poller> event_poller::create() {
    return std::make_unique<epoll_event_poller>();
}

} // namespace tang