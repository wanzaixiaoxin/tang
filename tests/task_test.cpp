#include "test_framework.h"
#include <tang/tang.h>
#include <atomic>
#include <thread>
#include <functional>

using namespace tang;
/**
 * Test basic coroutine creation and execution
 */
TEST(basic_task) {
    tang::RuntimeScope runtime(2);
    
    std::atomic_bool executed = false;
    
    // Create coroutine
    tang::go([&executed]() {
        executed = true;
    });
    
    // Run scheduler
    runtime.run();
    
    // Verify coroutine execution
    ASSERT_TRUE(executed.load());
}

/**
 * Test coroutine function return value
 */
TEST(task_return_value) {
    tang::RuntimeScope runtime(2);
    
    std::atomic_int result = 0;
    
    // Define a coroutine function that returns a value
    auto task_func = []() -> int {
        return 42;
    };
    
    // Create coroutine and capture result in atomic variable
    tang::go([&result, &task_func]() {
        result = task_func();
    });
    
    // Run scheduler
    runtime.run();
    
    // Verify return value
    ASSERT_EQUAL(42, result.load());
}

/**
 * Test multiple concurrent tasks
 */
TEST(multiple_tasks) {
    tang::RuntimeScope runtime(4);
    
    const int num_tasks = 10; // Reduce task count for faster test
    std::atomic_int executed_count = 0;
    
    // Create multiple coroutines
    for (int i = 0; i < num_tasks; ++i) {
        tang::go([&executed_count, i]() {
            // Simulate some work to be done
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            executed_count++;
        });
    }
    
    // Run scheduler
    runtime.run();
    
    // Verify all coroutines have been executed
    ASSERT_EQUAL(num_tasks, executed_count.load());
}

/**
 * Test task with exception handling
 */
TEST(task_exception_handling) {
    tang::RuntimeScope runtime(2);
    
    std::atomic_bool exception_caught = false;
    
    // Create coroutine that throws an exception
    tang::go([&exception_caught]() {
        try {
            throw std::runtime_error("Test exception");
        } catch (const std::exception&) {
            exception_caught = true;
        }
    });
    
    // Run scheduler
    runtime.run();
    
    // Verify exception was caught
    ASSERT_TRUE(exception_caught.load());
}

/**
 * Test task with sleep functionality
 */
TEST(task_with_sleep) {
    tang::RuntimeScope runtime(2);
    
    std::atomic_bool executed = false;
    auto start_time = std::chrono::steady_clock::now();
    
    // Create coroutine with sleep
    tang::go([&executed]() {
        // Sleep 50 milliseconds
        runtime::sleep_ms(50);
        executed = true;
    });
    
    // Run scheduler
    runtime.run();
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Verify execution
    ASSERT_TRUE(executed.load());
    ASSERT_TRUE(duration.count() >= 50); // Should take at least 50ms
}

/**
 * Test task with different thread counts
 */
TEST(task_different_thread_counts) {
    // Test with single thread
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
    
    // Test with multiple threads
    {
        tang::RuntimeScope runtime(4);
        std::atomic_int counter{0};
        
        for (int i = 0; i < 20; ++i) {
            tang::go([&counter]() {
                counter++;
            });
        }
        
        runtime.run();
        ASSERT_EQUAL(20, counter.load());
    }
}

/**
 * Main function using test framework
 */
int main(int argc, char* argv[]) {
    return tang::test::run_tests(argc, argv);
}