#include "test_framework.h"
#include <tang/tang.h>
#include <atomic>
#include <thread>
#include <functional>

/**
 * Test basic select operation
 */
TEST(basic_select) {
    tang::test::RuntimeScope runtime(2);
    
    // Create two channels
    tang::channel<int> ch1;
    tang::channel<int> ch2;
    
    std::atomic_int received = 0;
    std::atomic_int selected_channel = 0;
    
    // Create sender coroutine to send data to channel2
    tang::go([&ch2]() -> tang::task<void> {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        ch2 << 42;
        co_return;
    });
    
    // Use select to wait on both channels
    int value;
    
    tang::select(
        tang::case_recv(ch1, value, [&]() {
            received = value;
            selected_channel = 1;
        }),
        tang::case_recv(ch2, value, [&]() {
            received = value;
            selected_channel = 2;
        })
    );
    
    // Verify result
    ASSERT_EQUAL(42, received.load());
    ASSERT_EQUAL(2, selected_channel.load());
}

/**
 * Test select with default case
 */
TEST(select_with_default) {
    tang::test::RuntimeScope runtime(2);
    
    // Create a channel
    tang::channel<int> ch;
    
    std::atomic_bool default_executed = false;
    
    int value;
    
    tang::select(
        tang::case_recv(ch, value, [&]() {
            // This case won't be executed because no data is sent
        }),
        tang::default_case([&]() {
            default_executed = true;
        })
    );
    
    // Verify default case was executed
    ASSERT_TRUE(default_executed.load());
}

/**
 * Test select with multiple channels
 */
TEST(multiple_channels_select) {
    tang::test::RuntimeScope runtime(4);
    
    // Create three channels
    tang::channel<int> ch1;
    tang::channel<int> ch2;
    tang::channel<int> ch3;
    
    std::atomic_int received = 0;
    std::atomic_int received_count = 0;
    
    // Create three sender coroutines to send data to different channels
    tang::go([&ch1]() -> tang::task<void> {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        ch1 << 10;
        co_return;
    });
    
    tang::go([&ch2]() -> tang::task<void> {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ch2 << 20;
        co_return;
    });
    
    tang::go([&ch3]() -> tang::task<void> {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        ch3 << 30;
        co_return;
    });
    
    // Use select to wait on all channels
    int value;
    
    tang::select(
        tang::case_recv(ch1, value, [&]() {
            received = value;
            received_count++;
        }),
        tang::case_recv(ch2, value, [&]() {
            received = value;
            received_count++;
        }),
        tang::case_recv(ch3, value, [&]() {
            received = value;
            received_count++;
        })
    );
    
    // Verify result (should receive from ch2 first due to shortest delay)
    ASSERT_EQUAL(20, received.load());
    ASSERT_EQUAL(1, received_count.load());
}

/**
 * Test select with send operations
 */
TEST(select_with_send) {
    tang::test::RuntimeScope runtime(2);
    
    // Create channels
    tang::channel<int> ch1;
    tang::channel<int> ch2;
    
    std::atomic_int received = 0;
    std::atomic_int operation_type = 0; // 1 for send, 2 for recv
    
    // Create receiver coroutine for ch2
    tang::go([&ch2, &received]() -> tang::task<void> {
        int value;
        ch2 >> value;
        received = value;
        co_return;
    });
    
    // Use select with both send and receive operations
    int send_value = 100;
    int recv_value;
    
    tang::select(
        tang::case_send(ch1, send_value, [&]() {
            operation_type = 1;
        }),
        tang::case_recv(ch2, recv_value, [&]() {
            operation_type = 2;
            received = recv_value;
        })
    );
    
    // Verify result (should execute receive case since ch2 has receiver)
    ASSERT_EQUAL(2, operation_type.load());
    ASSERT_EQUAL(100, received.load());
}

/**
 * Test select with timeout using default case
 */
TEST(select_with_timeout) {
    tang::test::RuntimeScope runtime(2);
    
    // Create a channel
    tang::channel<int> ch;
    
    std::atomic_bool default_executed = false;
    std::atomic_bool recv_executed = false;
    
    int value;
    
    // Use select with default case (simulating timeout)
    tang::select(
        tang::case_recv(ch, value, [&]() {
            recv_executed = true;
        }),
        tang::default_case([&]() {
            default_executed = true;
        })
    );
    
    // Verify default case was executed (since no data is sent to channel)
    ASSERT_TRUE(default_executed.load());
    ASSERT_FALSE(recv_executed.load());
}

/**
 * Test select with mixed operations and priorities
 */
TEST(select_mixed_operations) {
    tang::test::RuntimeScope runtime(3);
    
    // Create channels
    tang::channel<int> ch1;
    tang::channel<std::string> ch2;
    tang::channel<double> ch3;
    
    std::atomic_int operation_count = 0;
    std::atomic_int int_value = 0;
    std::string str_value;
    std::mutex str_mutex;
    std::atomic<double> double_value{0.0};
    
    // Create sender coroutines
    tang::go([&ch1]() -> tang::task<void> {
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        ch1 << 42;
        co_return;
    });
    
    tang::go([&ch2]() -> tang::task<void> {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        ch2 << "test";
        co_return;
    });
    
    tang::go([&ch3]() -> tang::task<void> {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        ch3 << 3.14;
        co_return;
    });
    
    // Use select with mixed operations
    int int_val;
    std::string str_val;
    double double_val;
    
    tang::select(
        tang::case_recv(ch1, int_val, [&]() {
            int_value = int_val;
            operation_count++;
        }),
        tang::case_recv(ch2, str_val, [&]() {
            {
                std::lock_guard<std::mutex> lock(str_mutex);
                str_value = str_val;
            }
            operation_count++;
        }),
        tang::case_recv(ch3, double_val, [&]() {
            double_value = double_val;
            operation_count++;
        })
    );
    
    // Verify result (should receive from ch3 first due to shortest delay)
    ASSERT_EQUAL(1, operation_count.load());
    ASSERT_EQUAL(3.14, double_value.load());
}

/**
 * Test select with channel closure
 */
TEST(select_with_channel_closure) {
    tang::test::RuntimeScope runtime(2);
    
    // Create a channel and close it
    tang::channel<int> ch;
    ch.close();
    
    std::atomic_bool default_executed = false;
    std::atomic_bool recv_executed = false;
    
    int value;
    
    // Use select on closed channel
    tang::select(
        tang::case_recv(ch, value, [&]() {
            recv_executed = true;
        }),
        tang::default_case([&]() {
            default_executed = true;
        })
    );
    
    // Verify default case was executed due to closed channel
    ASSERT_TRUE(default_executed.load());
    ASSERT_FALSE(recv_executed.load());
}

/**
 * Test select with multiple operations in sequence
 */
TEST(select_multiple_operations_sequence) {
    tang::test::RuntimeScope runtime(2);
    
    tang::channel<int> ch1;
    tang::channel<int> ch2;
    
    std::atomic_int total_received = 0;
    std::vector<int> received_values;
    
    // Create sender coroutines
    tang::go([&ch1]() -> tang::task<void> {
        ch1 << 1;
        co_return;
    });
    
    tang::go([&ch2]() -> tang::task<void> {
        ch2 << 2;
        co_return;
    });
    
    // Perform multiple select operations
    int value;
    
    // First select
    tang::select(
        tang::case_recv(ch1, value, [&]() {
            received_values.push_back(value);
            total_received++;
        }),
        tang::case_recv(ch2, value, [&]() {
            received_values.push_back(value);
            total_received++;
        })
    );
    
    // Second select
    tang::select(
        tang::case_recv(ch1, value, [&]() {
            received_values.push_back(value);
            total_received++;
        }),
        tang::case_recv(ch2, value, [&]() {
            received_values.push_back(value);
            total_received++;
        })
    );
    
    // Verify both operations were executed
    ASSERT_EQUAL(2, total_received.load());
    ASSERT_EQUAL(2, received_values.size());
    
    // Values should be 1 and 2 (order may vary)
    ASSERT_TRUE(std::find(received_values.begin(), received_values.end(), 1) != received_values.end());
    ASSERT_TRUE(std::find(received_values.begin(), received_values.end(), 2) != received_values.end());
}

/**
 * Test select with complex nested operations
 */
TEST(select_complex_nested) {
    tang::test::RuntimeScope runtime(3);
    
    tang::channel<int> ch1;
    tang::channel<int> ch2;
    
    std::atomic_int final_result{0};
    
    // Create complex nested select scenario
    tang::go([&ch1, &ch2, &final_result]() -> tang::task<void> {
        int value1, value2;
        
        // First select
        tang::select(
            tang::case_recv(ch1, value1, [&]() {
                // Nested select inside callback
                tang::select(
                    tang::case_recv(ch2, value2, [&]() {
                        final_result = value1 + value2;
                    }),
                    tang::default_case([&]() {
                        final_result = value1 * 10;
                    })
                );
            }),
            tang::default_case([&]() {
                final_result = -1;
            })
        );
        
        co_return;
    });
    
    // Send data to trigger the complex scenario
    tang::go([&ch1]() -> tang::task<void> {
        ch1 << 5;
        co_return;
    });
    
    tang::go([&ch2]() -> tang::task<void> {
        ch2 << 3;
        co_return;
    });
    
    runtime.run();
    
    // Verify the complex operation completed
    ASSERT_EQUAL(8, final_result.load()); // 5 + 3
}

/**
 * Main function using test framework
 */
int main(int argc, char* argv[]) {
    return tang::test::run_tests(argc, argv);
}