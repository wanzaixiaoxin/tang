#include "test_framework.h"
#include <tang/tang.h>
#include <atomic>
#include <thread>

/**
 * Test basic coroutine functionality
 */
TEST(basic_coroutine) {
    tang::test::RuntimeScope runtime(1);
    
    std::atomic_bool executed = false;
    
    // Create a simple coroutine
    tang::go([&executed]() {
        executed = true;
    });
    
    // Run scheduler
    runtime.run();
    
    // Verify coroutine execution
    ASSERT_TRUE(executed.load());
}

/**
 * Test basic channel operation
 */
TEST(basic_channel_operation) {
    tang::test::RuntimeScope runtime(1);
    
    tang::channel<int> ch;
    int received = 0;
    
    // Create receiver coroutine
    tang::go([&ch, &received]() -> tang::task<void> {
        ch >> received;
        co_return;
    });
    
    // Send data
    ch << 42;
    
    // Run scheduler
    runtime.run();
    
    // Verify result
    ASSERT_EQUAL(42, received);
}

/**
 * Test channel with multiple operations
 */
TEST(channel_multiple_operations) {
    tang::test::RuntimeScope runtime(2);
    
    tang::channel<int> ch(5); // Buffered channel with capacity 5
    std::atomic_int received_count{0};
    
    // Create receiver coroutine
    tang::go([&ch, &received_count]() -> tang::task<void> {
        int value;
        for (int i = 0; i < 5; ++i) {
            ch >> value;
            received_count++;
        }
        co_return;
    });
    
    // Send multiple values
    for (int i = 0; i < 5; ++i) {
        ch << i;
    }
    
    // Run scheduler
    runtime.run();
    
    // Verify result
    ASSERT_EQUAL(5, received_count.load());
}

/**
 * Test channel closure
 */
TEST(channel_closure) {
    tang::test::RuntimeScope runtime(1);
    
    tang::channel<int> ch;
    std::atomic_bool receiver_finished{false};
    
    // Create receiver coroutine that should handle channel closure
    tang::go([&ch, &receiver_finished]() -> tang::task<void> {
        int value;
        bool result = ch >> value;
        ASSERT_FALSE(result); // Should return false when channel is closed
        receiver_finished = true;
        co_return;
    });
    
    // Close channel immediately
    ch.close();
    
    // Run scheduler
    runtime.run();
    
    // Verify receiver finished
    ASSERT_TRUE(receiver_finished.load());
}

/**
 * Test coroutine with sleep
 */
TEST(coroutine_with_sleep) {
    tang::test::RuntimeScope runtime(1);
    
    std::atomic_bool executed{false};
    auto start_time = std::chrono::steady_clock::now();
    
    // Create coroutine with sleep
    tang::go([&executed]() {
        // Sleep 100 milliseconds
        ::tang::runtime::sleep_ms(100);
        executed = true;
    });
    
    // Run scheduler
    runtime.run();
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Verify execution
    ASSERT_TRUE(executed.load());
    ASSERT_TRUE(duration.count() >= 100); // Should take at least 100ms
}

/**
 * Main function using test framework
 */
int main(int argc, char* argv[]) {
    return tang::test::run_tests(argc, argv);
}