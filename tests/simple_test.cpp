#include <iostream>
#include <cassert>
#include <tang/tang.h>
#include <vector>
#include <atomic>
#include <stdexcept>
#include <thread>

// Simple assertion macro
#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion failed: " << #condition << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::terminate(); \
        } \
    } while(0)

// Test basic channel operation
void test_basic_channel() {
    std::cout << "Testing basic channel operation..." << std::endl;
    // Create a channel
    tang::channel<int> ch;
    
    int received = 0;
    
    // Initialize runtime
    tang::runtime::init(2);
    
    // Create receiver coroutine
    tang::go([&ch, &received]() -> tang::task<void> {
        ch >> received;
        co_return;
    });
    
    // Send data
    ch << 42;
    
    // Run scheduler
    tang::runtime::run();
    
    // Verify result
    ASSERT(received == 42);
    
    std::cout << "Basic channel operation test passed!" << std::endl;
}

// Main function
int main() {
    std::cout << "Starting simple tests..." << std::endl;
    
    try {
        test_basic_channel();
        std::cout << "\nAll simple tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}