#include "test_framework.h"
#include <tang/tang.h>
#include <atomic>
#include <thread>
#include <chrono>

using namespace tang;
/**
 * Test single-threaded runtime initialization
 */
TEST(runtime_init_single_thread) {
    tang::RuntimeScope runtime(1);
    
    std::atomic_int counter{0};
    for (int i = 0; i < 10; ++i) {
        tang::go([&counter]() {
            counter++;
        });
    }
    
    runtime.run();
    ASSERT_EQUAL(10, counter.load());
}

/**
 * Test multi-threaded runtime initialization
 */
TEST(runtime_init_multi_thread) {
    tang::RuntimeScope runtime(4);
    
    std::atomic_int counter{0};
    for (int i = 0; i < 20; ++i) {
        tang::go([&counter]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            counter++;
        });
    }
    
    runtime.run();
    ASSERT_EQUAL(20, counter.load());
}

/**
 * Test runtime yield functionality
 */
TEST(runtime_yield) {
    tang::RuntimeScope runtime(2);
    
    // Simple test to verify yield functionality
    std::atomic<int> count{0};
    std::atomic<bool> task1_resumed{false};
    
    // Create a task that yields
    tang::go([&count, &task1_resumed]() {
        count++;
        runtime::yield(); // Yield execution
        task1_resumed = true;
        count++;
    });
    
    // Create a simple task
    tang::go([&count]() {
        count++;
    });
    
    runtime.run();
    
    // Both tasks should have run, count should be at least 2
    ASSERT_TRUE(count >= 2);
    // Task 1 should have been resumed after yield
    ASSERT_TRUE(task1_resumed);
}

/**
 * Test runtime sleep_ms functionality
 */
TEST(runtime_sleep_ms) {
    tang::RuntimeScope runtime(2);
    
    auto start = std::chrono::steady_clock::now();
    
    tang::go([]() {
        runtime::sleep_ms(50);
    });
    
    runtime.run();
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    ASSERT_TRUE(duration.count() >= 45);
}

/**
 * Test runtime with different thread counts
 */
TEST(runtime_different_thread_counts) {
    // Test with 1 thread
    {
        tang::RuntimeScope runtime(1);
        std::atomic_int counter{0};
        
        for (int i = 0; i < 5; ++i) {
            tang::go([&counter]() {
                counter++;
            });
        }
        
        runtime.run();
        ASSERT_EQUAL(5, counter.load());
    }
    
    // Test with 8 threads
    {
        tang::RuntimeScope runtime(8);
        std::atomic_int counter{0};
        
        for (int i = 0; i < 40; ++i) {
            tang::go([&counter]() {
                counter++;
            });
        }
        
        runtime.run();
        ASSERT_EQUAL(40, counter.load());
    }
}

/**
 * Test runtime with mixed operations
 */
TEST(runtime_mixed_operations) {
    tang::RuntimeScope runtime(4);
    
    std::atomic_int counter{0};
    std::atomic_bool sleep_completed{false};
    
    // Create tasks with different operations
    for (int i = 0; i < 10; ++i) {
        tang::go([&counter]() {
            counter++;
        });
    }
    
    tang::go([&sleep_completed]() {
        runtime::sleep_ms(30);
        sleep_completed = true;
    });
    
    runtime.run();
    
    ASSERT_EQUAL(10, counter.load());
    ASSERT_TRUE(sleep_completed.load());
}

/**
 * Test runtime stop and restart
 */
TEST(runtime_stop_restart) {
    // First run
    {
        tang::RuntimeScope runtime(2);
        std::atomic_int counter{0};
        
        for (int i = 0; i < 5; ++i) {
            tang::go([&counter]() {
                counter++;
            });
        }
        
        runtime.run();
        ASSERT_EQUAL(5, counter.load());
    }
    
    // Second run (should work independently)
    {
        tang::RuntimeScope runtime(3);
        std::atomic_int counter{0};
        
        for (int i = 0; i < 8; ++i) {
            tang::go([&counter]() {
                counter++;
            });
        }
        
        runtime.run();
        ASSERT_EQUAL(8, counter.load());
    }
}

/**
 * Main function using test framework
 */
int main(int argc, char* argv[]) {
    return tang::test::run_tests(argc, argv);
}