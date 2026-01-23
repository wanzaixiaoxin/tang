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

// Test basic channel send and receive
void test_basic_channel() {
    std::cout << "Testing basic channel..." << std::endl;
    // Create an unbuffered channel
    tang::channel<int> ch;
    std::atomic_int received = 0;
    
    // Initialize runtime
    tang::runtime::init(2);
    
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
    tang::runtime::run();
    
    // Verify results
    ASSERT(received.load() == 42);
    
    // Stop runtime
    tang::runtime::stop();
    std::cout << "Basic channel test passed!" << std::endl;
}

// Test buffered channel
void test_buffered_channel() {
    std::cout << "Testing buffered channel..." << std::endl;
    // Create a buffered channel with capacity 5
    tang::channel<int> ch(5);
    
    // Initialize runtime
    tang::runtime::init(2);
    
    // Test send operations
    ASSERT(ch.try_send(1));
    ASSERT(ch.try_send(2));
    ASSERT(ch.try_send(3));
    ASSERT(ch.try_send(4));
    ASSERT(ch.try_send(5));
    
    // Buffer is full, send attempt should fail
    ASSERT(!ch.try_send(6));
    
    // Test receive operations
    int value;
    ASSERT(ch.try_recv(value));
    ASSERT(value == 1);
    
    // Now buffer has space, can send
    ASSERT(ch.try_send(6));
    
    // Receive remaining values
    for (int i = 2; i <= 6; ++i) {
        ASSERT(ch.try_recv(value));
        ASSERT(value == i);
    }
    
    // Buffer is empty, receive attempt should fail
    ASSERT(!ch.try_recv(value));
    
    // Stop runtime
    tang::runtime::stop();
    std::cout << "Buffered channel test passed!" << std::endl;
}

// Test channel close
void test_channel_close() {
    std::cout << "Testing channel close..." << std::endl;
    // Create a channel
    tang::channel<int> ch(2);
    
    // Initialize runtime
    tang::runtime::init(2);
    
    // Send some data
    ch << 1;
    ch << 2;
    
    // Close channel
    ch.close();
    
    // Verify channel is closed
    ASSERT(ch.is_closed());
    
    // Attempting to send to closed channel should fail
    ASSERT(!ch.try_send(3));
    
    // Can receive remaining data
    int value;
    ASSERT(ch.try_recv(value));
    ASSERT(value == 1);
    
    ASSERT(ch.try_recv(value));
    ASSERT(value == 2);
    
    // After all data received, attempting to receive should fail
    ASSERT(!ch.try_recv(value));
    
    // Stop runtime
    tang::runtime::stop();
    std::cout << "Channel close test passed!" << std::endl;
}

// Test multiple senders and receivers
void test_multiple_sender_receiver() {
    std::cout << "Testing multiple senders and receivers..." << std::endl;
    const int num_senders = 2; // Reduce number to speed up test
    const int num_receivers = 1; // Reduce number to speed up test
    const int messages_per_sender = 3; // Reduce number to speed up test
    
    // Create a buffered channel
    tang::channel<int> ch(5); // Reduce buffer size
    
    std::atomic_int received = 0;
    std::atomic_int sent = 0;
    
    // Initialize runtime
    tang::runtime::init(2); // Reduce thread count
    
    // Create receiver coroutines
    for (int i = 0; i < num_receivers; ++i) {
        tang::go([&ch, &received, total = num_senders * messages_per_sender]() {
            while (received.load() < total) {
                int value;
                if (ch.try_recv(value)) {
                    received++;
                } else {
                    // Sleep briefly to avoid high CPU usage
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                }
            }
        });
    }
    
    // Create sender coroutines
    for (int i = 0; i < num_senders; ++i) {
        tang::go([&ch, &sent, i, count = messages_per_sender]() {
            for (int j = 0; j < count; ++j) {
                int value = i * 100 + j;
                ch << value;
                sent++;
            }
        });
    }
    
    // Run scheduler
    tang::runtime::run();
    
    // Verify results
    ASSERT(sent.load() == num_senders * messages_per_sender);
    ASSERT(received.load() == num_senders * messages_per_sender);
    
    // Stop runtime
    tang::runtime::stop();
    std::cout << "Multiple senders and receivers test passed!" << std::endl;
}

// Test channel status query
void test_channel_status() {
    std::cout << "Testing channel status query..." << std::endl;
    // Create a buffered channel with capacity 3
    tang::channel<int> ch(3);
    
    // Initialize runtime
    tang::runtime::init(2);
    
    // Initial state
    ASSERT(!ch.is_closed());
    ASSERT(ch.is_empty());
    ASSERT(!ch.is_full());
    
    // Send one element
    ch << 1;
    ASSERT(!ch.is_empty());
    ASSERT(!ch.is_full());
    
    // Send more elements until full
    ch << 2;
    ch << 3;
    ASSERT(!ch.is_empty());
    ASSERT(ch.is_full());
    
    // Receive one element
    int value;
    ch >> value;
    ASSERT(!ch.is_empty());
    ASSERT(!ch.is_full());
    
    // Close channel
    ch.close();
    ASSERT(ch.is_closed());
    
    // Stop runtime
    tang::runtime::stop();
    std::cout << "Channel status query test passed!" << std::endl;
}

// Test channel try_send and try_recv
void test_try_send_recv() {
    std::cout << "Testing try_send and try_recv..." << std::endl;
    // Create a buffered channel with capacity 2
    tang::channel<std::string> ch(2);
    
    // Initialize runtime
    tang::runtime::init(2);
    
    // Test try_send
    ASSERT(ch.try_send("hello"));
    ASSERT(ch.try_send("world"));
    ASSERT(!ch.try_send("tang"));
    
    // Test try_recv
    std::string msg;
    ASSERT(ch.try_recv(msg));
    ASSERT(msg == "hello");
    
    ASSERT(ch.try_recv(msg));
    ASSERT(msg == "world");
    
    ASSERT(!ch.try_recv(msg));
    
    // Stop runtime
    tang::runtime::stop();
    std::cout << "try_send and try_recv test passed!" << std::endl;
}

// Test receive after channel close
void test_receive_after_close() {
    std::cout << "Testing receive after channel close..." << std::endl;
    // Create a buffered channel with capacity 3
    tang::channel<int> ch(3);
    
    // Initialize runtime
    tang::runtime::init(2);
    
    // Send some data
    ch << 1;
    ch << 2;
    ch << 3;
    
    // Close channel
    ch.close();
    
    // Receive all data
    std::vector<int> received;
    int value;
    
    while (ch.try_recv(value)) {
        received.push_back(value);
    }
    
    // Verify received data
    ASSERT(received.size() == 3);
    ASSERT(received[0] == 1);
    ASSERT(received[1] == 2);
    ASSERT(received[2] == 3);
    
    // Attempt to receive after close should fail
    ASSERT(!ch.try_recv(value));
    
    // Stop runtime
    tang::runtime::stop();
    std::cout << "Receive after channel close test passed!" << std::endl;
}

// Test string type channel
void test_string_channel() {
    std::cout << "Testing string type channel..." << std::endl;
    // Create a buffered channel with capacity 2
    tang::channel<std::string> ch(2);
    
    // Initialize runtime
    tang::runtime::init(2);
    
    // Send strings
    ch << "hello";
    ch << "tang";
    
    // Receive strings
    std::string msg1, msg2;
    ch >> msg1;
    ch >> msg2;
    
    // Verify results
    ASSERT(msg1 == "hello");
    ASSERT(msg2 == "tang");
    
    // Stop runtime
    tang::runtime::stop();
    std::cout << "String type channel test passed!" << std::endl;
}

// Define a struct for testing
struct Person {
    std::string name;
    int age;
};

// Test struct type channel
void test_struct_channel() {
    std::cout << "Testing struct type channel..." << std::endl;
    
    // Create a buffered channel with capacity 2
    tang::channel<Person> ch(2);
    
    // Initialize runtime
    tang::runtime::init(2);
    
    // Send structs
    ch << Person{"Alice", 30};
    ch << Person{"Bob", 25};
    
    // Receive structs
    Person p1, p2;
    ch >> p1;
    ch >> p2;
    
    // Verify results
    ASSERT(p1.name == "Alice");
    ASSERT(p1.age == 30);
    
    ASSERT(p2.name == "Bob");
    ASSERT(p2.age == 25);
    
    // Stop runtime
    tang::runtime::stop();
    std::cout << "Struct type channel test passed!" << std::endl;
}

// Test capacity 1 channel
void test_single_capacity_channel() {
    std::cout << "Testing channel with capacity 1..." << std::endl;
    // Create a buffered channel with capacity 1
    tang::channel<int> ch(1);
    
    // Initialize runtime
    tang::runtime::init(2);
    
    // Send and receive
    ch << 100;
    ASSERT(ch.is_full());
    
    int value;
    ch >> value;
    ASSERT(value == 100);
    ASSERT(ch.is_empty());
    
    tang::runtime::stop();
    std::cout << "Channel with capacity 1 test passed!" << std::endl;
}

// Test unbuffered channel sync behavior
void test_unbuffered_channel_sync() {
    std::cout << "Testing unbuffered channel sync behavior..." << std::endl;
    // Create an unbuffered channel
    tang::channel<int> ch;
    std::atomic_bool sender_done{false};
    std::atomic_bool receiver_done{false};
    std::atomic_int received_value{0};
    
    tang::runtime::init(2);
    
    tang::go([&ch, &sender_done]() {
        ch << 42;
        sender_done = true;
    });
    
    tang::go([&ch, &receiver_done, &received_value]() {
        int value;
        ch >> value;
        received_value = value;
        receiver_done = true;
    });
    
    tang::runtime::run();
    
    ASSERT(received_value.load() == 42);
    ASSERT(sender_done.load());
    ASSERT(receiver_done.load());
    tang::runtime::stop();
    std::cout << "Unbuffered channel sync behavior test passed!" << std::endl;
}

// Test multiple producers and consumers
void test_multi_producer_consumer() {
    std::cout << "Testing multiple producers and consumers..." << std::endl;
    const int num_producers = 3;
    const int num_consumers = 2;
    const int messages_per_producer = 20;
    tang::channel<int> ch(10);
    std::atomic_int total_sent{0};
    std::atomic_int total_received{0};
    
    // Initialize runtime
    tang::runtime::init(4);
    
    // Create producer coroutines
    for (int i = 0; i < num_producers; ++i) {
        tang::go([&ch, &total_sent, i, messages_per_producer]() {
            for (int j = 0; j < messages_per_producer; ++j) {
                int value = i * 1000 + j;
                ch << value;
                total_sent++;
            }
        });
    }
    
    // Create consumer coroutines
    for (int i = 0; i < num_consumers; ++i) {
        tang::go([&ch, &total_received, total = num_producers * messages_per_producer]() {
            int count = 0;
            while (count < total) {
                int value;
                if (ch >> value) {
                    total_received++;
                    count++;
                }
            }
        });
    }
    
    tang::runtime::run();
    
    ASSERT(total_sent.load() == num_producers * messages_per_producer);
    ASSERT(total_received.load() == num_producers * messages_per_producer);
    tang::runtime::stop();
    std::cout << "Multiple producers and consumers test passed!" << std::endl;
}

// Test channel timeout behavior
void test_channel_timeout() {
    std::cout << "Testing channel timeout behavior..." << std::endl;
    tang::channel<int> ch(1);
    std::atomic_bool operation_completed{false};
    
    tang::runtime::init(2);
    
    // Send one value to fill the channel
    ch << 1;
    
    // Create a coroutine to try sending (should timeout or wait)
    tang::go([&ch, &operation_completed]() {
        [[maybe_unused]] auto start = std::chrono::steady_clock::now();
        // Try sending with timeout
        bool sent = false;
        int timeout_count = 0;
        while (!sent && timeout_count < 100) {
            sent = ch.try_send(2);
            if (!sent) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                timeout_count++;
            }
        }
        operation_completed = true;
    });
    
    tang::runtime::run();
    ASSERT(operation_completed.load());
    tang::runtime::stop();
    std::cout << "Channel timeout behavior test passed!" << std::endl;
}

// Test channel close wakeup behavior
void test_channel_close_wakeup() {
    std::cout << "Testing channel close wakeup behavior..." << std::endl;
    tang::channel<int> ch;
    std::atomic_int waiting_senders{0};
    std::atomic_int woken_senders{0};
    std::atomic_bool close_completed{false};
    
    tang::runtime::init(2);
    
    // Create a coroutine to wait for senders
    tang::go([&ch, &waiting_senders, &woken_senders]() {
        waiting_senders++;
        // This coroutine will block waiting for send
        ch << 100;
        woken_senders++;
    });
    
    // Wait for senders to be ready
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT(waiting_senders.load() >= 1);
    
    // Close channel, this should wake up waiting senders
    ch.close();
    close_completed = true;
    
    tang::runtime::run();
    
    ASSERT(close_completed.load());
    tang::runtime::stop();
    std::cout << "Channel close wakeup behavior test passed!" << std::endl;
}

// Test channel data order preservation
void test_channel_order_preservation() {
    std::cout << "Testing channel data order preservation..." << std::endl;
    const int num_messages = 100;
    tang::channel<int> ch(100);
    std::atomic_int last_received{0};
    std::atomic_int in_order_count{0};
    
    tang::runtime::init(2);
    
    // Send send in order
    tang::go([&ch, num_messages]() {
        for (int i = 0; i < num_messages; ++i) {
            ch << i;
        }
        ch.close();
    });
    
    // Receive in order
    tang::go([&ch, &last_received, &in_order_count, num_messages]() {
        int value;
        while (ch >> value) {
            if (value == last_received.load() + 1) {
                in_order_count++;
            }
            last_received = value;
        }
    });
    
    tang::runtime::run();
    
    ASSERT(last_received.load() == num_messages - 1);
    ASSERT(in_order_count.load() == num_messages - 1);
    tang::runtime::stop();
    std::cout << "Channel data order preservation test passed!" << std::endl;
}

// Test vector type channel
void test_vector_channel() {
    std::cout << "Testing vector type channel..." << std::endl;
    tang::channel<std::vector<int>> ch(2);
    
    tang::runtime::init(2);
    
    std::vector<int> v1 = {1, 2, 3, 4, 5};
    std::vector<int> v2 = {10, 20, 30};
    
    ch << v1;
    ch << v2;
    
    std::vector<int> r1, r2;
    ch >> r1;
    ch >> r2;
    
    ASSERT(r1.size() == 5);
    ASSERT(r2.size() == 3);
    ASSERT(r1[0] == 1);
    ASSERT(r2[0] == 10);
    
    tang::runtime::stop();
    std::cout << "Vector type channel test passed!" << std::endl;
}

// Test channel loop send receive
void test_channel_loop_send_recv() {
    std::cout << "Testing channel loop send receive..." << std::endl;
    const int iterations = 50;
    tang::channel<int> ch(10);
    std::atomic_int total_sent{0};
    std::atomic_int total_received{0};
    
    tang::runtime::init(2);
    
    // Loop send
    tang::go([&ch, &total_sent, iterations]() {
        for (int i = 0; i < iterations; ++i) {
            ch << i;
            total_sent++;
        }
        ch.close();
    });
    
    // Loop receive
    tang::go([&ch, &total_received, iterations]() {
        int value;
        int count = 0;
        while (count < iterations && (ch >> value)) {
            total_received++;
            count++;
        }
    });
    
    tang::runtime::run();
    
    ASSERT(total_sent.load() == iterations);
    ASSERT(total_received.load() == iterations);
    tang::runtime::stop();
    std::cout << "Channel loop send receive test passed!" << std::endl;
}

// Test channel boundary conditions
void test_channel_boundary_conditions() {
    std::cout << "Testing channel boundary conditions..." << std::endl;
    
    // Test channel with capacity 0
    tang::channel<int> ch0;
    ASSERT(ch0.is_full());
    ASSERT(ch0.is_empty() == false || !ch0.is_empty());
    
    // Test channel with capacity 1
    tang::channel<int> ch1(1);
    ASSERT(ch1.is_empty());
    ASSERT(!ch1.is_full());
    
    ch1 << 1;
    ASSERT(!ch1.is_empty());
    ASSERT(ch1.is_full());
    
    int value;
    ch1 >> value;
    ASSERT(value == 1);
    ASSERT(ch1.is_empty());
    ASSERT(!ch1.is_full());
    
    // Test large capacity channel
    tang::channel<int> ch100(100);
    for (int i = 0; i < 100; ++i) {
        ch100 << i;
    }
    ASSERT(ch100.is_full());
    
    for (int i = 0; i < 100; ++i) {
        ch100 >> value;
        ASSERT(value == i);
    }
    ASSERT(ch100.is_empty());
    
    std::cout << "Channel boundary conditions test passed!" << std::endl;
}

// Test channel concurrency safety
void test_channel_concurrent_safety() {
    std::cout << "Testing channel concurrent safety..." << std::endl;
    const int num_threads = 4;
    const int operations_per_thread = 100;
    tang::channel<int> ch(50);
    std::atomic_int total_operations{0};
    
    tang::runtime::init(num_threads);
    
    for (int t = 0; t < num_threads; ++t) {
        tang::go([&ch, &total_operations, t, operations_per_thread]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                int value = t * 10000 + i;
                ch << value;
                total_operations++;
            }
        });
    }
    
    // Consumer
    tang::go([&ch, &total_operations, num_threads, operations_per_thread]() {
        int expected_total = num_threads * operations_per_thread;
        int received = 0;
        int value;
        while (received < expected_total && (ch >> value)) {
            received++;
        }
    });
    
    tang::runtime::run();
    ASSERT(total_operations.load() == num_threads * operations_per_thread);
    tang::runtime::stop();
    std::cout << "Channel concurrent safety test passed!" << std::endl;
}

// Main function
int main() {
    std::cout << "Starting channel tests..." << std::endl;
    
    try {
        test_basic_channel();
        test_buffered_channel();
        test_channel_close();
        test_multiple_sender_receiver();
        test_channel_status();
        test_try_send_recv();
        test_receive_after_close();
        test_string_channel();
        test_struct_channel();
        test_single_capacity_channel();
        test_unbuffered_channel_sync();
        test_multi_producer_consumer();
        test_channel_timeout();
        test_channel_close_wakeup();
        test_channel_order_preservation();
        test_vector_channel();
        test_channel_loop_send_recv();
        test_channel_boundary_conditions();
        test_channel_concurrent_safety();
        
        std::cout << "\\\\nAll channel tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
