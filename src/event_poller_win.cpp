// Define NOMINMAX to prevent Windows max/min macros from conflicting with std::max/std::min
#define NOMINMAX

#include <tang/event_poller.h>
#include <tang/event.h>
#include <tang/event_source.h>
#include <windows.h>
#include <unordered_map>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>

namespace tang {

struct iocp_overlapped : public OVERLAPPED {
    event_source* source;
    uint32_t events;
};

class iocp_event_poller : public event_poller {
public:
    iocp_event_poller() : iocp_handle_(nullptr), shutdown_(false) {
        iocp_handle_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    }
    
    ~iocp_event_poller() override {
        shutdown_ = true;
        if (iocp_handle_ != nullptr) {
            CloseHandle(iocp_handle_);
            iocp_handle_ = nullptr;
        }
    }
    
    int wait(std::vector<event>& events, std::chrono::milliseconds timeout) override {
        DWORD bytes_transferred;
        ULONG_PTR completion_key;
        LPOVERLAPPED overlapped;
        
        DWORD wait_time = (timeout == std::chrono::milliseconds::max()) ? INFINITE : static_cast<DWORD>(timeout.count());
        
        BOOL result = GetQueuedCompletionStatus(
            iocp_handle_,
            &bytes_transferred,
            &completion_key,
            &overlapped,
            wait_time
        );
        
        if (result && overlapped != nullptr) {
            // Successfully got completion status
            iocp_overlapped* iocp_ov = static_cast<iocp_overlapped*>(overlapped);
            event_source* source = iocp_ov->source;
            uint32_t event_type = iocp_ov->events;
            
            event e;
            e.source = source;
            e.events = event_type;
            events.push_back(e);
            
            return 1;
        } else if (overlapped != nullptr) {
            // Operation failed but has overlapped structure info
            iocp_overlapped* iocp_ov = static_cast<iocp_overlapped*>(overlapped);
            event_source* source = iocp_ov->source;
            
            event e;
            e.source = source;
            e.events = static_cast<uint32_t>(event_type::error);
            events.push_back(e);
            
            return 1;
        } else if (!result) {
            // No overlapped structure, check for timeout
            DWORD last_error = GetLastError();
            if (last_error == WAIT_TIMEOUT) {
                return 0;
            }
            // Other error
            return -1;
        }
        
        return 0;
    }
    
    bool register_event(event_source* source, uint32_t events) override {
        if (source == nullptr || iocp_handle_ == nullptr) {
            return false;
        }
        
        HANDLE handle = static_cast<HANDLE>(source->get_handle());
        if (handle == nullptr || handle == INVALID_HANDLE_VALUE) {
            return false;
        }
        
        // Associate handle with IOCP
        HANDLE io_result = CreateIoCompletionPort(handle, iocp_handle_, reinterpret_cast<ULONG_PTR>(source), 0);
        if (io_result == nullptr) {
            return false;
        }
        
        return true;
    }
    
    bool modify_event(event_source* source, uint32_t events) override {
        return register_event(source, events);
    }
    
    bool delete_event(event_source* source) override {
        return true;
    }
    
private:
    HANDLE iocp_handle_;
    std::atomic<bool> shutdown_;
};

std::unique_ptr<event_poller> event_poller::create() {
    return std::make_unique<iocp_event_poller>();
}

} // namespace tang