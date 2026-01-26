#include "test_framework.h"
#include <tang/tang.h>
#include <atomic>
#include <thread>

/**
 * Test basic coroutine functionality
 */
TEST2(basic_coroutine,true) {
    tang::RuntimeScope runtime(1);

    std::atomic_bool executed = false;

    LOG_INFO(tang::logger::test, "Before creating coroutine, executed = " + std::to_string(executed.load()));

    try {
        // Create a simple coroutine
        tang::go([&executed]() {
            LOG_INFO(tang::logger::test, "Inside coroutine, setting executed to true");
            executed = true;
            LOG_INFO(tang::logger::test, "Inside coroutine, executed = " + std::to_string(executed.load()));
        });

        LOG_INFO(tang::logger::test, "After creating coroutine, executed = " + std::to_string(executed.load()));

        // Give some time for coroutine to be scheduled
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // Run scheduler
        runtime.run();

        LOG_INFO(tang::logger::test, "After runtime.run(), executed = " + std::to_string(executed.load()));

        // Verify coroutine execution
        ASSERT_TRUE(executed.load());
    } catch (const std::exception& e) {
        LOG_ERROR(tang::logger::test, "Exception in basic_coroutine: " + std::string(e.what()));
        throw;
    } catch (...) {
        LOG_ERROR(tang::logger::test, "Unknown exception in basic_coroutine");
        throw;
    }
}

/**
 * Test basic channel operation
 */
TEST2(basic_channel_operation,true) {
    tang::RuntimeScope runtime(1);
    
    // Use buffered channel to avoid deadlock
    tang::channel<int> ch(1);
    int received = 0;
    
    // Create sender coroutine - like Go goroutine
    tang::go([&ch]() -> tang::task<void> {
        LOG_INFO(tang::logger::test, "Inside sender coroutine");
        co_await ch.send(42);  // Block until receiver is ready (Go-like behavior)
        LOG_INFO(tang::logger::test, "Sent value: 42");
        co_return;
    });
    
    // Create receiver coroutine - like Go goroutine
    tang::go([&ch, &received]() -> tang::task<void> {
        LOG_INFO(tang::logger::test, "Inside receiver coroutine");
        co_await ch.recv(received);  // Block until data is available (Go-like behavior)
        LOG_INFO(tang::logger::test, "Received value: " + std::to_string(received));
        co_return;
    });
    
    // Run scheduler once to process both coroutines - like Go runtime
    LOG_INFO(tang::logger::test, "Starting scheduler for sender and receiver");
    runtime.run();
    
    LOG_INFO(tang::logger::test, "After runtime.run(), received = " + std::to_string(received));
    // Verify result
    ASSERT_EQUAL(42, received);
}

/**
 * Test channel with multiple operations
 */
TEST2(channel_multiple_operations,true) {
    tang::RuntimeScope runtime(2);
    
    tang::channel<int> ch(5); // Buffered channel with capacity 5
    std::atomic_int received_count{0};
    
    // Create receiver coroutine
    tang::go([&ch, &received_count]() -> tang::task<void> {
        LOG_INFO(tang::logger::test, "Inside receiver coroutine");
        for (int i = 0; i < 5; ++i) {
            int value;
            bool result = co_await ch.recv(value);
            LOG_INFO(tang::logger::test, "co_await ch.recv returned: " + std::to_string(result) + ", value: " + std::to_string(value));
            received_count++; 
            LOG_INFO(tang::logger::test, "received_count: " + std::to_string(received_count.load()));
        }
        LOG_INFO(tang::logger::test, "Received 5 values");
        co_return;
    });
        
    // Create sender coroutine - like Go goroutine
    tang::go([&ch]() -> tang::task<void> {
        LOG_INFO(tang::logger::test, "Inside sender coroutine");
        for (int i = 0; i < 5; ++i) {
            LOG_DEBUG(tang::logger::test, "Sending value: " + std::to_string(i));
            co_await ch.send(i);
            LOG_DEBUG(tang::logger::test, "Send completed for value: " + std::to_string(i));
        }
        LOG_INFO(tang::logger::test, "Sent 5 values");
        co_return;
    });
    
    // Run scheduler to process all operations
    LOG_INFO(tang::logger::test, "Starting scheduler for multiple operations");
    runtime.run();
    LOG_INFO(tang::logger::test, "After runtime.run(), received_count = " + std::to_string(received_count.load()));
    
    // Verify result
    ASSERT_EQUAL(5, received_count.load());
}

/**
 * Test channel closure
 */
TEST2(channel_closure,true) {
    tang::RuntimeScope runtime(1);
    
    tang::channel<int> ch;
    std::atomic_bool receiver_finished{false};
    
    // Create receiver coroutine that should handle channel closure
    tang::go([&ch, &receiver_finished]() -> tang::task<void> {
        int value;
        bool result = co_await ch.recv(value);
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
TEST2(coroutine_with_sleep,true) {
    tang::RuntimeScope runtime(1);
    
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
    tang::logger::init();
    return tang::test::run_tests(argc, argv);
}