#include <iostream>
#include <cassert>
#include <tang/tang.h>
#include <tang/logger.h>
#include <vector>
#include <atomic>
#include <stdexcept>
#include <thread>
#include <string>
#include <sstream>

// Simple assertion macro
#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            LOG_ERROR(tang::logger::test, std::string("Assertion failed: ") + #condition + " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
            std::terminate(); \
        } \
    } while(0)

// Test basic channel operation
void test_basic_channel() {
    LOG_INFO(tang::logger::test, "Testing basic channel operation...");
    
    // Simple test without channels first
    std::atomic_bool executed = false;
    
    // Initialize runtime
    tang::runtime::init(1);
    
    // Create a simple coroutine
    tang::go([&executed]() {
        LOG_DEBUG(tang::logger::test, "Simple coroutine started");
        executed = true;
        LOG_DEBUG(tang::logger::test, "Simple coroutine completed");
    });
    
    // Run scheduler
    tang::runtime::run();
    
    // Verify coroutine execution
    ASSERT(executed.load());
    
    LOG_INFO(tang::logger::test, "Basic coroutine test passed!");
    
    // Now test with channels
    tang::channel<int> ch;
    int received = 0;
    
    // Re-initialize runtime
    tang::runtime::init(1);
    
    // Create receiver coroutine
    tang::go([&ch, &received]() -> tang::task<void> {
        LOG_DEBUG(tang::logger::test, "Receiver coroutine started");
        ch >> received;
        
        std::stringstream msg;
        msg << "Receiver coroutine received: " << received;
        LOG_DEBUG(tang::logger::test, msg.str());
        
        co_return;
    });
    
    // Send data
    ch << 42;
    
    // Run scheduler
    tang::runtime::run();
    
    // Verify result
    ASSERT(received == 42);
    
    LOG_INFO(tang::logger::test, "Basic channel operation test passed!");
}

// Main function
int main() {
    LOG_INFO(tang::logger::test, "Starting simple tests...");
    
    try {
        test_basic_channel();
        LOG_INFO(tang::logger::test, "\nAll simple tests passed!");
        return 0;
    } catch (const std::exception& e) {
        LOG_ERROR(tang::logger::test, std::string("Test failed: ") + e.what());
        return 1;
    }
}