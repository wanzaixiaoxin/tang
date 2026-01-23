#include "test_framework.h"
#include <tang/tang.h>
#include <atomic>
#include <thread>
#include <vector>

/**
 * Test basic channel send and receive
 */
TEST(basic_channel) {
    tang::RuntimeScope runtime(2);
    
    // Create an unbuffered channel
    tang::channel<int> ch;
    std::atomic_int received = 0;
    
    // Create receiver coroutine
    tang::go([&ch, &received]() {
        int value;
        ch >> value;
        received = value;
    });
    
    // Create sender coroutine
    tang::go([&ch]() {
        ch << 42;
    });
    
    // Run scheduler
    runtime.run();
    
    // Verify results
    ASSERT_EQUAL(42, received.load());
}

/**
 * Test buffered channel
 */
TEST(buffered_channel) {
    tang::RuntimeScope runtime(2);
    
    // Create a buffered channel with capacity 5
    tang::channel<int> ch(5);
    
    // Test send operations
    ASSERT_TRUE(ch.try_send(1));
    ASSERT_TRUE(ch.try_send(2));
    ASSERT_TRUE(ch.try_send(3));
    ASSERT_TRUE(ch.try_send(4));
    ASSERT_TRUE(ch.try_send(5));
    
    // Buffer is full, send attempt should fail
    ASSERT_FALSE(ch.try_send(6));
    
    // Test receive operations
    int value;
    ASSERT_TRUE(ch.try_recv(value));
    ASSERT_EQUAL(1, value);
    
    // Now buffer has space, can send
    ASSERT_TRUE(ch.try_send(6));
    
    // Receive remaining values
    for (int i = 2; i <= 6; ++i) {
        ASSERT_TRUE(ch.try_recv(value));
        ASSERT_EQUAL(i, value);
    }
    
    // Buffer is empty, receive attempt should fail
    ASSERT_FALSE(ch.try_recv(value));
}

/**
 * Test channel close
 */
TEST(channel_close) {
    tang::RuntimeScope runtime(2);
    
    // Create a channel
    tang::channel<int> ch(2);
    
    // Send some values
    ASSERT_TRUE(ch.try_send(1));
    ASSERT_TRUE(ch.try_send(2));
    
    // Close the channel
    ch.close();
    
    // Should not be able to send after close
    ASSERT_FALSE(ch.try_send(3));
    
    // Should be able to receive remaining values
    int value;
    ASSERT_TRUE(ch.try_recv(value));
    ASSERT_EQUAL(1, value);
    
    ASSERT_TRUE(ch.try_recv(value));
    ASSERT_EQUAL(2, value);
    
    // Should not be able to receive after all values are consumed
    ASSERT_FALSE(ch.try_recv(value));
}

/**
 * Test channel with multiple producers and consumers
 */
TEST(channel_multiple_producers_consumers) {
    tang::RuntimeScope runtime(4);
    
    tang::channel<int> ch(10);
    std::atomic_int total_received{0};
    const int num_producers = 3;
    const int num_consumers = 2;
    const int items_per_producer = 10;
    
    // Create producer coroutines
    for (int i = 0; i < num_producers; ++i) {
        tang::go([&ch, i]() {
            for (int j = 0; j < items_per_producer; ++j) {
                ch << (i * 100 + j);
            }
        });
    }
    
    // Create consumer coroutines
    for (int i = 0; i < num_consumers; ++i) {
        tang::go([&ch, &total_received]() {
            int value;
            for (int j = 0; j < (num_producers * items_per_producer) / num_consumers; ++j) {
                ch >> value;
                total_received++;
            }
        });
    }
    
    // Run scheduler
    runtime.run();
    
    // Verify all items were processed
    ASSERT_EQUAL(num_producers * items_per_producer, total_received.load());
}

/**
 * Test channel with blocking operations
 */
TEST(channel_blocking_operations) {
    tang::RuntimeScope runtime(2);
    
    tang::channel<int> ch;
    std::atomic_int received_value{0};
    std::atomic_bool receiver_started{false};
    std::atomic_bool sender_completed{false};
    
    // Create receiver coroutine
    tang::go([&ch, &received_value, &receiver_started]() {
        receiver_started = true;
        int value;
        ch >> value;  // This should block until sender sends
        received_value = value;
    });
    
    // Wait for receiver to start
    while (!receiver_started.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Create sender coroutine
    tang::go([&ch, &sender_completed]() {
        ch << 100;  // This should unblock the receiver
        sender_completed = true;
    });
    
    // Run scheduler
    runtime.run();
    
    // Verify operations completed
    ASSERT_TRUE(sender_completed.load());
    ASSERT_EQUAL(100, received_value.load());
}

/**
 * Test channel with different data types
 */
TEST(channel_different_data_types) {
    tang::RuntimeScope runtime(2);
    
    // Test with string
    {
        tang::channel<std::string> str_ch;
        std::string received_str;
        std::mutex str_mutex;
        
        tang::go([&str_ch, &received_str, &str_mutex]() {
            std::string value;
            str_ch >> value;
            {
                std::lock_guard<std::mutex> lock(str_mutex);
                received_str = value;
            }
        });
        
        tang::go([&str_ch]() {
            str_ch << "Hello, World!";
        });
        
        runtime.run();
        {
            std::lock_guard<std::mutex> lock(str_mutex);
            ASSERT_EQUAL("Hello, World!", received_str);
        }
    }
    
    // Test with vector
    {
        tang::channel<std::vector<int>> vec_ch;
        std::vector<int> received_vec;
        std::mutex vec_mutex;
        
        tang::go([&vec_ch, &received_vec, &vec_mutex]() {
            std::vector<int> value;
            vec_ch >> value;
            {
                std::lock_guard<std::mutex> lock(vec_mutex);
                received_vec = value;
            }
        });
        
        tang::go([&vec_ch]() {
            vec_ch << std::vector<int>{1, 2, 3, 4, 5};
        });
        
        runtime.run();
        {
            std::lock_guard<std::mutex> lock(vec_mutex);
            ASSERT_EQUAL(5, received_vec.size());
            ASSERT_EQUAL(1, received_vec[0]);
            ASSERT_EQUAL(5, received_vec[4]);
        }
    }
}

/**
 * Test channel capacity limits
 */
TEST(channel_capacity_limits) {
    tang::RuntimeScope runtime(2);
    
    // Test zero capacity (unbuffered)
    {
        tang::channel<int> ch(0);
        ASSERT_FALSE(ch.try_send(1));  // Should fail without receiver
    }
    
    // Test small capacity
    {
        tang::channel<int> ch(1);
        ASSERT_TRUE(ch.try_send(1));
        ASSERT_FALSE(ch.try_send(2));  // Should fail - buffer full
        
        int value;
        ASSERT_TRUE(ch.try_recv(value));
        ASSERT_EQUAL(1, value);
        ASSERT_TRUE(ch.try_send(2));   // Should succeed now
    }
    
    // Test large capacity
    {
        tang::channel<int> ch(100);
        for (int i = 0; i < 100; ++i) {
            ASSERT_TRUE(ch.try_send(i));
        }
        ASSERT_FALSE(ch.try_send(100));  // Should fail - buffer full
        
        int value;
        for (int i = 0; i < 100; ++i) {
            ASSERT_TRUE(ch.try_recv(value));
            ASSERT_EQUAL(i, value);
        }
        ASSERT_FALSE(ch.try_recv(value));  // Should fail - buffer empty
    }
}

/**
 * Main function using test framework
 */
int main(int argc, char* argv[]) {
    return tang::test::run_tests(argc, argv);
}