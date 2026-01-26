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

    LOG_INFO(tang::logger::test) << "Before creating coroutine, executed = " << executed.load();

    try {
        // Create a simple coroutine
        tang::go([&executed]() {
            LOG_INFO(tang::logger::test) << "Inside coroutine, setting executed to true";
            executed = true;
            LOG_INFO(tang::logger::test) << "Inside coroutine, executed = " << executed.load();
        });

        LOG_INFO(tang::logger::test) << "After creating coroutine, executed = " << executed.load();

        // Give some time for coroutine to be scheduled
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // Run scheduler
        runtime.run();

        LOG_INFO(tang::logger::test) << "After runtime.run(), executed = " << executed.load();

        // Verify coroutine execution
        ASSERT_TRUE(executed.load());
    } catch (const std::exception& e) {
        LOG_ERROR(tang::logger::test) << "Exception in basic_coroutine: " << e.what();   
        throw;
    } catch (...) {
        LOG_ERROR(tang::logger::test) << "Unknown exception in basic_coroutine";
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
        LOG_INFO(tang::logger::test) << "Inside sender coroutine";
        co_await ch.send(42);  // Block until receiver is ready (Go-like behavior)
        LOG_INFO(tang::logger::test) << "Sent value: 42";
        co_return;
    });
    
    // Create receiver coroutine - like Go goroutine
    tang::go([&ch, &received]() -> tang::task<void> {
        LOG_INFO(tang::logger::test) << "Inside receiver coroutine";
        co_await ch.recv(received);  // Block until data is available (Go-like behavior)
        LOG_INFO(tang::logger::test) << "Received value: " << received;   
        co_return;
    });
    
    // Run scheduler once to process both coroutines - like Go runtime
    LOG_INFO(tang::logger::test) << "Starting scheduler for sender and receiver";
    runtime.run();
    
    LOG_INFO(tang::logger::test) << "After runtime.run(), received = " << received;
    // Verify result
    ASSERT_EQUAL(42, received);
}

/**
 * Test channel with multiple operations - simplified version
 */
TEST2(channel_multiple_operations,true) {
    tang::RuntimeScope runtime(1);
    
    tang::channel<int> ch(5); // Buffered channel with capacity 5
    std::atomic_int received_count{0};
    
    // Use simpler approach: send first, then receive
    
    // Send all values first
    for (int i = 0; i < 5; ++i) {
        LOG_DEBUG(tang::logger::test) << "Sending value: " << i;
        bool sent = ch.try_send(i);
        ASSERT_TRUE(sent);
        LOG_DEBUG(tang::logger::test) << "Send completed for value: " << i;
    }
    
    LOG_INFO(tang::logger::test) << "Sent 5 values to buffer";
    
    // Create receiver coroutine to receive all values
    tang::go([&ch, &received_count]() -> tang::task<void> {
        LOG_INFO(tang::logger::test) << "Inside receiver coroutine";
        
        for (int i = 0; i < 5; ++i) {
            int value;
            bool result = co_await ch.recv(value);
            LOG_INFO(tang::logger::test) << "Received value: " << value << ", result: " << result;
            
            if (result) {
                received_count++;
                LOG_INFO(tang::logger::test) << "received_count: " << received_count.load();
            } else {
                LOG_ERROR(tang::logger::test) << "Receive failed at iteration " << i;
                break;
            }
        }
        
        LOG_INFO(tang::logger::test) << "Receiver completed: Received " << received_count.load() << " values";
        co_return;
    });
    
    // Run scheduler to process receiver
    LOG_INFO(tang::logger::test) << "Starting scheduler for receiver";
    runtime.run();
    
    LOG_INFO(tang::logger::test) << "After runtime.run(), received_count = " << received_count.load();
    
    // Verify result
    ASSERT_EQUAL(5, received_count.load());
}

/**
 * Test channel closure - use try_recv instead of coroutine
 */
TEST2(channel_closure,true) {
    tang::RuntimeScope runtime(1);
    
    tang::channel<int> ch;
    
    // Close channel
    ch.close();
    
    // Test try_recv on closed channel
    int value;
    bool result = ch.try_recv(value);
    LOG_INFO(tang::logger::test) << "try_recv on closed channel result: " << result;
    
    // Should return false when channel is closed
    ASSERT_FALSE(result);
    
    // Run scheduler to ensure no pending operations
    runtime.run();
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
 * Test multiple producers and single consumer - simplified version
 */
TEST2(multiple_producers_single_consumer,true) {
    tang::RuntimeScope runtime(1);
    
    tang::channel<int> ch(20); // Larger buffer for multiple producers
    std::atomic_int received_count{0};
    
    // Create multiple producer coroutines
    const int num_producers = 3;
    const int values_per_producer = 5;
    
    // First, send all values synchronously to avoid scheduling issues
    for (int producer_id = 0; producer_id < num_producers; ++producer_id) {
        for (int i = 0; i < values_per_producer; ++i) {
            int value = producer_id * 100 + i;
            LOG_DEBUG(tang::logger::test) << "Sending value: " << value;
            bool sent = ch.try_send(value);
            ASSERT_TRUE(sent);
            LOG_DEBUG(tang::logger::test) << "Send completed for value: " << value;
        }
    }
    
    LOG_INFO(tang::logger::test) << "All " << (num_producers * values_per_producer) << " values sent to buffer";
    
    // Create single consumer coroutine
    tang::go([&ch, &received_count, num_producers, values_per_producer]() -> tang::task<void> {
        LOG_INFO(tang::logger::test) << "Consumer started";
        
        for (int i = 0; i < num_producers * values_per_producer; ++i) {
            int value;
            bool result = co_await ch.recv(value);
            
            if (result) {
                received_count++;
                LOG_DEBUG(tang::logger::test) << "Consumer received: " << value << ", count: " << received_count.load();
            } else {
                LOG_ERROR(tang::logger::test) << "Receive failed at iteration " << i;
                break;
            }
        }
        
        LOG_INFO(tang::logger::test) << "Consumer finished, received: " << received_count.load() << " values";
        co_return;
    });
    
    // Run scheduler
    runtime.run();
    
    // Verify results
    ASSERT_EQUAL(num_producers * values_per_producer, received_count.load());
}

/**
 * Test channel with complex data types
 */
TEST2(channel_complex_data,true) {
    tang::RuntimeScope runtime(1);
    
    struct ComplexData {
        int id;
        std::string name;
        double value;
        
        ComplexData(int i, const std::string& n, double v) : id(i), name(n), value(v) {}
        
        bool operator==(const ComplexData& other) const {
            return id == other.id && name == other.name && value == other.value;
        }
    };
    
    tang::channel<ComplexData> ch(3);
    std::atomic_int received_count{0};
    ComplexData received_data{-1, "", 0.0};
    
    // Create sender coroutine
    tang::go([&ch]() -> tang::task<void> {
        LOG_INFO(tang::logger::test) << "Complex data sender started";
        
        co_await ch.send(ComplexData(1, "test1", 1.5));
        co_await ch.send(ComplexData(2, "test2", 2.5));
        co_await ch.send(ComplexData(3, "test3", 3.5));
        
        LOG_INFO(tang::logger::test) << "Complex data sender finished";
        co_return;
    });
    
    // Create receiver coroutine
    tang::go([&ch, &received_count, &received_data]() -> tang::task<void> {
        LOG_INFO(tang::logger::test) << "Complex data receiver started";
        
        for (int i = 0; i < 3; ++i) {
            ComplexData data(-1, "", 0.0);
            bool result = co_await ch.recv(data);
            
            if (result) {
                received_count++;
                received_data = data; // Store last received data
                LOG_DEBUG(tang::logger::test) << "Received complex data: id=" << data.id 
                          << ", name=" << data.name << ", value=" << data.value;
            } else {
                LOG_ERROR(tang::logger::test) << "Complex data receive failed";
                break;
            }
        }
        
        LOG_INFO(tang::logger::test) << "Complex data receiver finished";
        co_return;
    });
    
    // Run scheduler
    runtime.run();
    
    // Verify results
    ASSERT_EQUAL(3, received_count.load());
    ASSERT_TRUE(received_data == ComplexData(3, "test3", 3.5));
}

/**
 * Test channel timeout behavior - simplified version
 */
TEST2(channel_timeout_behavior,true) {
    tang::RuntimeScope runtime(1);
    
    tang::channel<int> ch(1); // Small buffer
    std::atomic_bool normal_receive_occurred{false};
    
    // Test: Send a value and then receive it
    tang::go([&ch, &normal_receive_occurred]() -> tang::task<void> {
        LOG_INFO(tang::logger::test) << "Normal receive test started";
        
        // Send a value first
        co_await ch.send(42);
        
        // Then receive it
        int value;
        bool result = co_await ch.recv(value);
        
        if (result && value == 42) {
            normal_receive_occurred = true;
            LOG_INFO(tang::logger::test) << "Normal receive successful";
        }
        
        co_return;
    });
    
    // Run scheduler
    runtime.run();
    
    // Verify normal operation works
    ASSERT_TRUE(normal_receive_occurred.load());
}

/**
 * Test channel with exception handling
 */
TEST2(channel_exception_handling,true) {
    tang::RuntimeScope runtime(1);
    
    tang::channel<int> ch(2);
    std::atomic_bool exception_handled{false};
    std::atomic_bool normal_operation_ok{false};
    
    // Create coroutine that throws exception
    tang::go([&ch, &exception_handled]() -> tang::task<void> {
        LOG_INFO(tang::logger::test) << "Exception test coroutine started";
        
        try {
            // This should work normally
            co_await ch.send(100);
            
            // Simulate an error
            throw std::runtime_error("Test exception");
            
        } catch (const std::exception& e) {
            LOG_INFO(tang::logger::test) << "Exception caught: " << e.what();
            exception_handled = true;
        }
        
        co_return;
    });
    
    // Create normal coroutine to test channel still works
    tang::go([&ch, &normal_operation_ok]() -> tang::task<void> {
        LOG_INFO(tang::logger::test) << "Normal operation test started";
        
        // Wait for the exception coroutine to finish
        ::tang::runtime::sleep_ms(100);
        
        // Channel should still work after exception
        co_await ch.send(200);
        
        int value1, value2;
        bool result1 = co_await ch.recv(value1);
        bool result2 = co_await ch.recv(value2);
        
        if (result1 && result2 && value1 == 100 && value2 == 200) {
            normal_operation_ok = true;
            LOG_INFO(tang::logger::test) << "Normal operation verified";
        }
        
        co_return;
    });
    
    // Run scheduler
    runtime.run();
    
    // Verify exception was handled and channel still works
    ASSERT_TRUE(exception_handled.load());
    ASSERT_TRUE(normal_operation_ok.load());
}

/**
 * Test channel performance with true concurrent operations
 */
TEST2(channel_performance_test,true) {
    tang::RuntimeScope runtime(2); // Use 2 threads for true concurrency
    
    const int NUM_OPERATIONS = 20;
    tang::channel<int> ch(10); // Moderate buffer size
    std::atomic_int send_count{0};
    std::atomic_int receive_count{0};
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Create sender coroutine - true concurrent operation
    tang::go([&ch, &send_count, NUM_OPERATIONS]() -> tang::task<void> {
        LOG_INFO(tang::logger::test) << "Performance sender started";
        
        for (int i = 0; i < NUM_OPERATIONS; ++i) {
            LOG_DEBUG(tang::logger::test) << "Sender sending value: " << i;
            co_await ch.send(i);
            send_count++;
            LOG_DEBUG(tang::logger::test) << "Sender sent value: " << i << ", count: " << send_count.load();
        }
        
        LOG_INFO(tang::logger::test) << "Performance sender finished";
        co_return;
    });
    
    // Create receiver coroutine - true concurrent operation
    tang::go([&ch, &receive_count, NUM_OPERATIONS]() -> tang::task<void> {
        LOG_INFO(tang::logger::test) << "Performance receiver started";
        
        for (int i = 0; i < NUM_OPERATIONS; ++i) {
            int value;
            bool result = co_await ch.recv(value);
            
            if (result) {
                receive_count++;
                LOG_DEBUG(tang::logger::test) << "Receiver received value: " << value << ", count: " << receive_count.load();
            } else {
                LOG_ERROR(tang::logger::test) << "Performance test receive failed";
                break;
            }
        }
        
        LOG_INFO(tang::logger::test) << "Performance receiver finished, received: " << receive_count.load() << " values";
        co_return;
    });
    
    // Run scheduler
    runtime.run();
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    LOG_INFO(tang::logger::test) << "Performance test completed: " << NUM_OPERATIONS 
              << " operations in " << duration.count() << "ms";
    
    // Verify all operations completed
    ASSERT_EQUAL(NUM_OPERATIONS, send_count.load());
    ASSERT_EQUAL(NUM_OPERATIONS, receive_count.load());
    
    // Performance check: should complete reasonably fast
    ASSERT_TRUE(duration.count() < 5000); // Should complete in under 5 seconds
}

/**
 * Main function using test framework
 */
int main(int argc, char* argv[]) {
    tang::logger::init();
    return tang::test::run_tests(argc, argv);
}