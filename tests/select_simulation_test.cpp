#include "test_framework.h"
#include <tang/tang.h>
#include <atomic>
#include <thread>
#include <chrono>

using namespace tang;

/**
 * Test basic channel selection logic (simulating select behavior)
 */
TEST(select_simulation_basic) {
    tang::RuntimeScope runtime(2);
    
    tang::channel<int> ch1(1);
    tang::channel<int> ch2(1);
    std::atomic_int result{0};
    std::atomic_bool selected{false};
    
    // Send data to ch1 first
    ch1 << 100;
    
    // Simulate select by checking both channels
    tang::go([&ch1, &ch2, &result, &selected]() {
        int value;
        // Check ch1 first (simulating select priority)
        if (ch1.try_recv(value)) {
            result = value;
            selected = true;
            LOG_INFO(logger::test) << "Selected ch1 with value: " << value;
        } else if (ch2.try_recv(value)) {
            result = value + 1000;
            selected = true;
            LOG_INFO(logger::test) << "Selected ch2 with value: " << value;
        }
    });
    
    runtime.run();
    
    ASSERT_TRUE(selected.load());
    ASSERT_EQUAL(100, result.load());
}

/**
 * Test multiple channel selection
 */
TEST(select_multiple_channels) {
    tang::RuntimeScope runtime(2);
    
    tang::channel<int> ch1(2);
    tang::channel<int> ch2(2);
    tang::channel<int> ch3(2);
    std::atomic_int selections_from_ch1{0};
    std::atomic_int selections_from_ch2{0};
    std::atomic_int selections_from_ch3{0};
    
    // Fill all channels
    ch1 << 1;
    ch1 << 2;
    ch2 << 10;
    ch2 << 20;
    ch3 << 100;
    ch3 << 200;
    
    const int iterations = 6;
    
    tang::go([&ch1, &ch2, &ch3, &selections_from_ch1, &selections_from_ch2, &selections_from_ch3, iterations]() {
        for (int i = 0; i < iterations; ++i) {
            int value;
            // Round-robin style selection simulation
            if (ch1.try_recv(value)) {
                selections_from_ch1++;
                LOG_DEBUG(logger::test) << "Selected from ch1: " << value;
            } else if (ch2.try_recv(value)) {
                selections_from_ch2++;
                LOG_DEBUG(logger::test) << "Selected from ch2: " << value;
            } else if (ch3.try_recv(value)) {
                selections_from_ch3++;
                LOG_DEBUG(logger::test) << "Selected from ch3: " << value;
            }
        }
    });
    
    runtime.run();
    
    int total_selections = selections_from_ch1.load() + selections_from_ch2.load() + selections_from_ch3.load();
    ASSERT_EQUAL(iterations, total_selections);
    
    LOG_INFO(logger::test) << "Selection distribution - ch1: " << selections_from_ch1.load()
              << ", ch2: " << selections_from_ch2.load()
              << ", ch3: " << selections_from_ch3.load();
}

/**
 * Test select-like behavior with timeout
 */
TEST(select_timeout_simulation) {
    tang::RuntimeScope runtime(2);
    
    tang::channel<int> ch(1);
    std::atomic_bool operation_completed{false};
    std::atomic_bool timeout_occurred{false};
    auto start_time = std::chrono::steady_clock::now();
    
    tang::go([&ch, &operation_completed, &timeout_occurred]() {
        int value;
        // Try to receive with timeout simulation
        const int timeout_ms = 100;
        auto check_start = std::chrono::steady_clock::now();
        
        while (std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - check_start).count() < timeout_ms) {
            if (ch.try_recv(value)) {
                operation_completed = true;
                LOG_INFO(logger::test) << "Received value: " << value;
                return;
            }
            // Brief pause to avoid busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        // Timeout occurred
        timeout_occurred = true;
        LOG_INFO(logger::test) << "Timeout occurred after " << timeout_ms << "ms";
    });
    
    runtime.run();
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    ASSERT_TRUE(timeout_occurred.load());
    ASSERT_FALSE(operation_completed.load());
    ASSERT_TRUE(duration.count() >= 100);
}

/**
 * Test fairness in channel selection
 */
TEST(select_fairness_test) {
    tang::RuntimeScope runtime(2);
    
    tang::channel<int> ch1(10);
    tang::channel<int> ch2(10);
    std::atomic_int ch1_selections{0};
    std::atomic_int ch2_selections{0};
    
    // Pre-fill channels with different amounts
    for (int i = 0; i < 7; ++i) ch1 << i;
    for (int i = 0; i < 3; ++i) ch2 << i + 100;
    
    const int total_operations = 10;
    
    tang::go([&ch1, &ch2, &ch1_selections, &ch2_selections, total_operations]() {
        for (int i = 0; i < total_operations; ++i) {
            int value;
            // Alternate checking order to test fairness
            bool check_ch1_first = (i % 2 == 0);
            
            if (check_ch1_first) {
                if (ch1.try_recv(value)) {
                    ch1_selections++;
                } else if (ch2.try_recv(value)) {
                    ch2_selections++;
                }
            } else {
                if (ch2.try_recv(value)) {
                    ch2_selections++;
                } else if (ch1.try_recv(value)) {
                    ch1_selections++;
                }
            }
        }
    });
    
    runtime.run();
    
    int total_selections = ch1_selections.load() + ch2_selections.load();
    ASSERT_EQUAL(total_operations, total_selections);
    
    // Both channels should have been selected from
    ASSERT_TRUE(ch1_selections.load() > 0);
    ASSERT_TRUE(ch2_selections.load() > 0);
    
    LOG_INFO(logger::test) << "Fairness test - ch1 selections: " << ch1_selections.load()
              << ", ch2 selections: " << ch2_selections.load();
}

/**
 * Test select behavior with closed channels
 */
TEST(select_closed_channel_handling) {
    tang::RuntimeScope runtime(2);
    
    tang::channel<int> active_ch(1);
    tang::channel<int> closed_ch(1);
    std::atomic_bool selected_from_active{false};
    std::atomic_bool default_case_executed{false};
    
    // Close one channel
    closed_ch.close();
    // Send data to active channel
    active_ch << 42;
    
    tang::go([&active_ch, &closed_ch, &selected_from_active, &default_case_executed]() {
        int value;
        // Try to select from channels
        if (active_ch.try_recv(value)) {
            selected_from_active = true;
            LOG_INFO(logger::test) << "Selected from active channel: " << value;
        } else if (!closed_ch.is_closed() && closed_ch.try_recv(value)) {
            // This shouldn't happen since channel is closed
            LOG_ERROR(logger::test) << "Should not select from closed channel";
        } else {
            default_case_executed = true;
            LOG_INFO(logger::test) << "Default case executed for closed channel scenario";
        }
    });
    
    runtime.run();
    
    ASSERT_TRUE(selected_from_active.load());
    ASSERT_FALSE(default_case_executed.load()); // Should select from active channel
}

/**
 * Test select performance characteristics
 */
TEST(select_performance_characteristics) {
    tang::RuntimeScope runtime(4);
    
    const int operations = 100;
    tang::channel<int> ch1(50);
    tang::channel<int> ch2(50);
    std::atomic_int total_processed{0};
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Pre-fill channels
    for (int i = 0; i < operations/2; ++i) {
        ch1 << i;
        ch2 << i + 1000;
    }
    
    tang::go([&ch1, &ch2, &total_processed, operations]() {
        for (int i = 0; i < operations; ++i) {
            int value;
            if (ch1.try_recv(value)) {
                total_processed++;
            } else if (ch2.try_recv(value)) {
                total_processed++;
            }
        }
    });
    
    runtime.run();
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    ASSERT_EQUAL(operations, total_processed.load());
    ASSERT_TRUE(duration.count() < 1000); // Should complete quickly
    
    LOG_INFO(logger::test) << "Select performance: " << operations
              << " operations in " << duration.count() << "ms";
}

/**
 * Main function
 */
int main(int argc, char* argv[]) {
    return tang::test::run_tests(argc, argv);
}