#include <iostream>
#include <cassert>
#include <tang/tang.h>
#include <vector>
#include <atomic>
#include <stdexcept>
#include <thread>
#include <functional>

// Simple assertion macro
#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion failed: " << #condition << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::terminate(); \
        } \
    } while(0)

// Test basic select operation
tang::task<void> test_basic_select() {
    std::cout << "Testing basic select operation..." << std::endl;
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
    ASSERT(received.load() == 42);
    ASSERT(selected_channel.load() == 2);
    
    std::cout << "Basic select operation test passed!" << std::endl;
    co_return;
}

// Test select with default case
tang::task<void> test_select_with_default() {
    std::cout << "Testing select with default case..." << std::endl;
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
    ASSERT(default_executed.load());
    
    std::cout << "Select with default case test passed!" << std::endl;
    co_return;
}

// Test select with multiple channels
tang::task<void> test_multiple_channels_select() {
    std::cout << "Testing select with multiple channels..." << std::endl;
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
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        ch2 << 20;
        co_return;
    });
    
    tang::go([&ch3]() -> tang::task<void> {
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        ch3 << 30;
        co_return;
    });
    
    const int total = 3;
    while (received_count.load() < total) {
        int value;
        
        tang::select(
            tang::case_recv(ch1, value, [&]() {
                received += value;
                received_count++;
            }),
            tang::case_recv(ch2, value, [&]() {
                received += value;
                received_count++;
            }),
            tang::case_recv(ch3, value, [&]() {
                received += value;
                received_count++;
            })
        );
    }
    
    // Verify result
    ASSERT(received.load() == 60);
    ASSERT(received_count.load() == 3);
    
    std::cout << "Select with multiple channels test passed!" << std::endl;
    co_return;
}

// Test select send case
tang::task<void> test_select_send_case() {
    std::cout << "Testing select send case..." << std::endl;
    // Create two channels
    tang::channel<int> ch1(1);
    tang::channel<int> ch2(1);
    
    std::atomic_int sent_value = 0;
    std::atomic_int selected_channel = 0;
    
    // First send data to ch1 to make it full
    ch1 << 100;
    
    // Create receiver coroutine to receive from ch1, allowing further sends
    tang::go([&ch1]() -> tang::task<void> {
        int value;
        ch1 >> value;
        co_return;
    });
    
    // Use select to choose which channel to send to
    int value = 42;
    
    tang::select(
        tang::case_send(ch1, value, [&]() {
            sent_value = value;
            selected_channel = 1;
        }),
        tang::case_send(ch2, value, [&]() {
            sent_value = value;
            selected_channel = 2;
        })
    );
    
    // Verify result
    ASSERT(sent_value.load() == 42);
    // May choose channel1 or channel2, so use OR condition
    ASSERT(selected_channel.load() == 1 || selected_channel.load() == 2);
    
    std::cout << "Select send case test passed!" << std::endl;
    co_return;
}

// Test select fairness
tang::task<void> test_select_fairness() {
    std::cout << "Testing select fairness..." << std::endl;
    // Create two channels
    tang::channel<int> ch1;
    tang::channel<int> ch2;
    
    std::atomic_int ch1_count = 0;
    std::atomic_int ch2_count = 0;
    const int total = 100; // Increase test count to better verify fairness
    
    // Create two sender coroutines to send data to both channels simultaneously
    tang::go([&ch1, total]() -> tang::task<void> {
        for (int i = 0; i < total / 2; ++i) {
            ch1 << i;
        }
        co_return;
    });
    
    tang::go([&ch2, total]() -> tang::task<void> {
        for (int i = 0; i < total / 2; ++i) {
            ch2 << i;
        }
        co_return;
    });
    
    // Use select to wait on both channels, count selections for each channel
    int value;
    for (int i = 0; i < total; ++i) {
        tang::select(
            tang::case_recv(ch1, value, [&]() {
                ch1_count++;
            }),
            tang::case_recv(ch2, value, [&]() {
                ch2_count++;
            })
        );
    }
    
    // Verify result: both channels should be selected roughly equally (considering scheduling uncertainty)
    ASSERT(ch1_count.load() + ch2_count.load() == total);
    
    // Calculate deviation to ensure fairness (allow 10% deviation)
    int expected = total / 2;
    int diff = std::abs(static_cast<int>(ch1_count.load()) - expected);
    double tolerance = expected * 0.1;
    ASSERT(diff <= tolerance);
    
    std::cout << "Select fairness test passed!" << std::endl;
    std::cout << "ch1_count: " << ch1_count.load() << ", ch2_count: " << ch2_count.load() << std::endl;
    co_return;
}

// Test select with channel close
tang::task<void> test_select_with_channel_close() {
    std::cout << "Testing select with channel close..." << std::endl;
    tang::channel<int> ch1;
    tang::channel<int> ch2;
    std::atomic_int received_value = 0;
    std::atomic_bool close_handled = false;
    
    tang::go([&ch1]() -> tang::task<void> {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        ch1 << 100;
        co_return;
    });
    
    tang::go([&ch2, &close_handled]() -> tang::task<void> {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ch2.close();
        close_handled = true;
        co_return;
    });
    
    int value;
    tang::select(
        tang::case_recv(ch1, value, [&]() {
            received_value = value;
        }),
        tang::case_recv(ch2, value, [&]() {
            // Closed channel should not be selected
            received_value = -1;
        })
    );
    
    ASSERT(received_value.load() == 100);
    ASSERT(close_handled.load());
    
    std::cout << "Select with channel close test passed!" << std::endl;
    co_return;
}

// Test select timeout behavior
tang::task<void> test_select_timeout() {
    std::cout << "Testing select timeout behavior..." << std::endl;
    tang::channel<int> ch;
    std::atomic_bool default_executed = false;
    int timeout_count = 0;
    int value = 0;
    
    for (int i = 0; i < 5; ++i) {
        tang::select(
            tang::case_recv(ch, value, [&]() {}),
            tang::default_case([&]() {
                default_executed = true;
                timeout_count++;
            })
        );
    }
    
    ASSERT(timeout_count == 5);
    ASSERT(default_executed.load());
    
    std::cout << "Select timeout behavior test passed!" << std::endl;
    co_return;
}

// Test select with many channels
tang::task<void> test_select_many_channels() {
    std::cout << "Testing select with many channels..." << std::endl;
    // Don't use vector, use multiple independent channel variables directly
    std::atomic_int selected_channel = -1;
    
    // Use independent channel array instead of vector
    tang::channel<int> ch1, ch2, ch3, ch4, ch5, ch6, ch7, ch8, ch9, ch10;
    tang::channel<int>* channels[] = {&ch1, &ch2, &ch3, &ch4, &ch5, &ch6, &ch7, &ch8, &ch9, &ch10};
    
    // Randomly select a channel to send data
    int target_channel = 5;
    tang::go([&channels, target_channel]() -> tang::task<void> {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        *channels[target_channel] << 42;
        co_return;
    });
    
    int value = 0;
    tang::select(
        tang::case_recv(*channels[0], value, [&]() { selected_channel = 0; }),
        tang::case_recv(*channels[1], value, [&]() { selected_channel = 1; }),
        tang::case_recv(*channels[2], value, [&]() { selected_channel = 2; }),
        tang::case_recv(*channels[3], value, [&]() { selected_channel = 3; }),
        tang::case_recv(*channels[4], value, [&]() { selected_channel = 4; }),
        tang::case_recv(*channels[5], value, [&]() { selected_channel = 5; }),
        tang::case_recv(*channels[6], value, [&]() { selected_channel = 6; }),
        tang::case_recv(*channels[7], value, [&]() { selected_channel = 7; }),
        tang::case_recv(*channels[8], value, [&]() { selected_channel = 8; }),
        tang::case_recv(*channels[9], value, [&]() { selected_channel = 9; })
    );
    
    ASSERT(selected_channel.load() == target_channel);
    ASSERT(value == 42);
    
    std::cout << "Select with many channels test passed!" << std::endl;
    co_return;
}

// Test exception handling in select
tang::task<void> test_select_exception_handling() {
    std::cout << "Testing exception handling in select..." << std::endl;
    tang::channel<int> ch;
    std::atomic_bool exception_caught = false;
    std::atomic_bool callback_executed = false;
    
    tang::go([&ch]() -> tang::task<void> {
        ch << 100;
        co_return;
    });
    
    int value;
    try {
        tang::select(
            tang::case_recv(ch, value, [&]() {
                if (value > 50) {
                    throw std::runtime_error("Value too large");
                }
                callback_executed = true;
            })
        );
    } catch (const std::exception& e) {
        exception_caught = true;
    }
    
    ASSERT(exception_caught.load());
    
    std::cout << "Exception handling in select test passed!" << std::endl;
    co_return;
}

// Test select with mixed recv and send
tang::task<void> test_select_mixed_recv_send() {
    std::cout << "Testing select with mixed recv and send..." << std::endl;
    tang::channel<int> ch1(1);
    tang::channel<int> ch2;
    std::atomic_int operation_type = -1;
    
    ch1 << 10;
    
    tang::go([&ch2]() -> tang::task<void> {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ch2 << 20;
        co_return;
    });
    
    int value;
    tang::select(
        tang::case_recv(ch1, value, [&]() {
            operation_type = 0;
        }),
        tang::case_send(ch1, 30, [&]() {
            operation_type = 1;
        }),
        tang::case_recv(ch2, value, [&]() {
            operation_type = 2;
        })
    );
    
    ASSERT(operation_type.load() >= 0 && operation_type.load() <= 2);
    
    std::cout << "Select with mixed recv and send test passed!" << std::endl;
    co_return;
}

// Test nested select usage
tang::task<void> test_select_nested() {
    std::cout << "Testing nested select usage..." << std::endl;
    tang::channel<int> ch1;
    tang::channel<int> ch2;
    std::atomic_int result = 0;
    
    tang::go([&ch1]() -> tang::task<void> {
        ch1 << 1;
        co_return;
    });
    
    tang::go([&ch2]() -> tang::task<void> {
        ch2 << 2;
        co_return;
    });
    
    int value1, value2;
    tang::select(
        tang::case_recv(ch1, value1, [&]() {
            result = value1;
            tang::select(
                tang::case_recv(ch2, value2, [&]() {
                    result += value2;
                }),
                tang::default_case([&]() {})
            );
        }),
        tang::case_recv(ch2, value2, [&]() {
            result = value2;
        })
    );
    
    ASSERT(result.load() == 3);
    
    std::cout << "Nested select usage test passed!" << std::endl;
    co_return;
}

// Test select with multiple selections
tang::task<void> test_select_multiple_selections() {
    std::cout << "Testing select with multiple selections..." << std::endl;
    tang::channel<int> ch1;
    tang::channel<int> ch2;
    std::atomic_int select_count = 0;
    std::atomic_int sum = 0;
    
    tang::go([&ch1, &ch2]() -> tang::task<void> {
        for (int i = 0; i < 5; ++i) {
            ch1 << i;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        ch1.close();
        ch2.close();
        co_return;
    });
    
    int value;
    while (true) {
        tang::select(
            tang::case_recv(ch1, value, [&]() {
                select_count++;
                sum += value;
            }),
            tang::default_case([&]() {
                if (select_count.load() >= 5) {
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            })
        );
        if (select_count.load() >= 5) {
            break;
        }
    }
    
    ASSERT(select_count.load() == 5);
    ASSERT(sum.load() == 10);
    
    std::cout << "Select with multiple selections test passed!" << std::endl;
    co_return;
}

// Test runner
tang::task<void> run_tests() {
    co_await test_basic_select();
    co_await test_select_with_default();
    co_await test_multiple_channels_select();
    co_await test_select_send_case();
    co_await test_select_fairness();
    co_await test_select_with_channel_close();
    co_await test_select_timeout();
    co_await test_select_many_channels();
    co_await test_select_exception_handling();
    co_await test_select_mixed_recv_send();
    co_await test_select_nested();
    co_await test_select_multiple_selections();
    co_return;
}

// Main function
int main() {
    std::cout << "Starting select tests..." << std::endl;
    
    try {
        // Initialize runtime
        tang::runtime::init(2);
        
        // Run all tests
        auto test_task = run_tests();
        test_task.run();
        
        // Run scheduler
        tang::runtime::run();
        
        std::cout << "\nAll select tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}