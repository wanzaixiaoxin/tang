#include "test_framework.h"
#include <tang/tang.h>
#include <atomic>
#include <thread>
#include <chrono>

using namespace tang;

/**
 * Test basic defer functionality
 */
TEST(defer_basic_functionality) {
    tang::RuntimeScope runtime(1);
    
    std::atomic_bool defer_executed{false};
    std::atomic_bool main_function_completed{false};
    
    tang::go([&defer_executed, &main_function_completed]() {
        LOG_INFO(logger::test) << "Entering function with defer";
        
        defer {
            defer_executed = true;
            LOG_INFO(logger::test) << "Defer block executed";
        } end_defer
        
        LOG_INFO(logger::test) << "Main function logic executing";
        main_function_completed = true;
        
        // Function completes normally
        LOG_INFO(logger::test) << "Function about to return normally";
    });
    
    runtime.run();
    
    ASSERT_TRUE(main_function_completed.load());
    ASSERT_TRUE(defer_executed.load());
    LOG_INFO(logger::test) << "Basic defer test passed";
}

/**
 * Test defer with early return
 */
TEST(defer_early_return) {
    tang::RuntimeScope runtime(1);
    
    std::atomic_bool defer_executed{false};
    std::atomic_int execution_point{0};
    
    tang::go([&defer_executed, &execution_point]() {
        LOG_INFO(logger::test) << "Function with potential early return";
        
        defer {
            defer_executed = true;
            LOG_INFO(logger::test) << "Defer executed after early return";
        } end_defer
        
        execution_point = 1;
        LOG_INFO(logger::test) << "Point 1 reached";
        
        // Early return condition
        if (true) {
            execution_point = 2;
            LOG_INFO(logger::test) << "Early return at point 2";
            return;
        }
        
        execution_point = 3; // This should not be reached
        LOG_INFO(logger::test) << "Point 3 reached (should not happen)";
    });
    
    runtime.run();
    
    ASSERT_EQUAL(2, execution_point.load()); // Should stop at point 2
    ASSERT_TRUE(defer_executed.load()); // Defer should still execute
    LOG_INFO(logger::test) << "Early return defer test passed";
}

/**
 * Test defer with exception
 */
TEST(defer_exception_handling) {
    tang::RuntimeScope runtime(1);
    
    std::atomic_bool defer_executed{false};
    std::atomic_bool exception_caught{false};
    std::atomic_bool defer_after_exception{false};
    
    tang::go([&defer_executed, &exception_caught, &defer_after_exception]() {
        try {
            LOG_INFO(logger::test) << "Function that throws exception";
            
            defer {
                defer_executed = true;
                LOG_INFO(logger::test) << "First defer executed";
            } end_defer
            
            // Throw exception
            throw std::runtime_error("Test exception");
            
            defer {
                defer_after_exception = true; // This should not execute
                LOG_INFO(logger::test) << "This defer should not execute";
            } end_defer
            
        } catch (const std::exception& e) {
            exception_caught = true;
            LOG_INFO(logger::test) << "Exception caught: " << e.what();
        }
    });
    
    runtime.run();
    
    ASSERT_TRUE(exception_caught.load());
    ASSERT_TRUE(defer_executed.load()); // First defer should execute
    ASSERT_FALSE(defer_after_exception.load()); // Second defer should not execute
    LOG_INFO(logger::test) << "Exception handling defer test passed";
}

/**
 * Test multiple defers in same scope
 */
TEST(defer_multiple_in_scope) {
    tang::RuntimeScope runtime(1);
    
    std::atomic_int defer_execution_order{0};
    std::atomic_int defer1_executed{0};
    std::atomic_int defer2_executed{0};
    std::atomic_int defer3_executed{0};
    
    tang::go([&defer_execution_order, &defer1_executed, &defer2_executed, &defer3_executed]() {
        LOG_INFO(logger::test) << "Function with multiple defers";
        
        defer {
            defer_execution_order = 1;
            defer1_executed = 1;
            LOG_INFO(logger::test) << "First defer executed (should be last)";
        } end_defer
        
        defer {
            defer_execution_order = 2;
            defer2_executed = 2;
            LOG_INFO(logger::test) << "Second defer executed (should be middle)";
        } end_defer
        
        defer {
            defer_execution_order = 3;
            defer3_executed = 3;
            LOG_INFO(logger::test) << "Third defer executed (should be first)";
        } end_defer
        
        LOG_INFO(logger::test) << "Main function logic completed";
    });
    
    runtime.run();
    
    // Defers should execute in reverse order (LIFO)
    ASSERT_EQUAL(1, defer_execution_order.load()); // Last defer should execute first
    ASSERT_EQUAL(1, defer1_executed.load());
    ASSERT_EQUAL(2, defer2_executed.load());
    ASSERT_EQUAL(3, defer3_executed.load());
    LOG_INFO(logger::test) << "Multiple defer test passed";
}

/**
 * Test defer with resource management
 */
TEST(defer_resource_management) {
    tang::RuntimeScope runtime(1);
    
    std::atomic_bool resource_acquired{false};
    std::atomic_bool resource_released{false};
    std::atomic_int resource_state{0}; // 0=initial, 1=acquired, 2=released
    
    tang::go([&resource_acquired, &resource_released, &resource_state]() {
        LOG_INFO(logger::test) << "Resource management function starting";
        
        // Acquire resource
        resource_acquired = true;
        resource_state = 1;
        LOG_INFO(logger::test) << "Resource acquired";
        
        defer {
            // Release resource
            resource_released = true;
            resource_state = 2;
            LOG_INFO(logger::test) << "Resource released by defer";
        } end_defer
        
        // Use resource
        LOG_INFO(logger::test) << "Using resource...";
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        LOG_INFO(logger::test) << "Resource usage completed";
    });
    
    runtime.run();
    
    ASSERT_TRUE(resource_acquired.load());
    ASSERT_TRUE(resource_released.load());
    ASSERT_EQUAL(2, resource_state.load()); // Should be in released state
    LOG_INFO(logger::test) << "Resource management defer test passed";
}

/**
 * Test defer in loop context
 */
TEST(defer_in_loop_context) {
    tang::RuntimeScope runtime(1);
    
    std::atomic_int defer_count{0};
    std::atomic_int loop_iterations{0};
    
    tang::go([&defer_count, &loop_iterations]() {
        LOG_INFO(logger::test) << "Loop with defer test starting";
        
        for (int i = 0; i < 3; ++i) {
            loop_iterations++;
            LOG_INFO(logger::test) << "Loop iteration: " << i;
            
            defer {
                defer_count++;
                LOG_INFO(logger::test) << "Defer executed for iteration: " << i;
            } end_defer
            
            // Each iteration should have its own defer scope
        }
        
        LOG_INFO(logger::test) << "Loop completed";
    });
    
    runtime.run();
    
    ASSERT_EQUAL(3, loop_iterations.load());
    ASSERT_EQUAL(3, defer_count.load()); // Each iteration should execute defer
    LOG_INFO(logger::test) << "Loop defer test passed";
}

/**
 * Test defer with conditional logic
 */
TEST(defer_conditional_logic) {
    tang::RuntimeScope runtime(1);
    
    std::atomic_bool defer_should_execute{true};
    std::atomic_bool defer_executed{false};
    
    tang::go([&defer_should_execute, &defer_executed]() {
        LOG_INFO(logger::test) << "Conditional defer test starting";
        
        if (defer_should_execute) {
            defer {
                defer_executed = true;
                LOG_INFO(logger::test) << "Conditional defer executed";
            } end_defer
            
            LOG_INFO(logger::test) << "Condition was true, defer registered";
        } else {
            LOG_INFO(logger::test) << "Condition was false, no defer";
        }
        
        LOG_INFO(logger::test) << "Function logic completed";
    });
    
    runtime.run();
    
    ASSERT_TRUE(defer_executed.load());
    LOG_INFO(logger::test) << "Conditional defer test passed";
}

/**
 * Test defer with coroutine suspension
 */
TEST(defer_coroutine_suspension) {
    tang::RuntimeScope runtime(2);
    
    std::atomic_bool defer_executed{false};
    std::atomic_bool coroutine_resumed{false};
    
    tang::go([&defer_executed, &coroutine_resumed]() -> tang::task<void> {
        LOG_INFO(logger::test) << "Coroutine with defer starting";
        
        defer {
            defer_executed = true;
            LOG_INFO(logger::test) << "Coroutine defer executed";
        } end_defer
        
        LOG_INFO(logger::test) << "Coroutine about to suspend";
        
        // Suspend coroutine
        runtime::yield();
        coroutine_resumed = true;
        
        LOG_INFO(logger::test) << "Coroutine resumed";
        co_return;
    });
    
    runtime.run();
    
    ASSERT_TRUE(coroutine_resumed.load());
    ASSERT_TRUE(defer_executed.load());
    LOG_INFO(logger::test) << "Coroutine suspension defer test passed";
}

/**
 * Test defer performance overhead
 */
TEST(defer_performance_overhead) {
    tang::RuntimeScope runtime(1);
    
    const int iterations = 1000;
    std::atomic_int normal_counter{0};
    std::atomic_int defer_counter{0};
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Test normal function calls
    tang::go([&normal_counter, iterations]() {
        for (int i = 0; i < iterations; ++i) {
            normal_counter++;
        }
    });
    
    // Test function calls with defer
    tang::go([&defer_counter, iterations]() {
        for (int i = 0; i < iterations; ++i) {
            defer {
                // Empty defer for performance testing
            } end_defer
            defer_counter++;
        }
    });
    
    runtime.run();
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    ASSERT_EQUAL(iterations, normal_counter.load());
    ASSERT_EQUAL(iterations, defer_counter.load());
    
    LOG_INFO(logger::test) << "Defer performance test: " << iterations 
              << " iterations in " << duration.count() << "ms";
    
    // Performance check - should complete in reasonable time
    ASSERT_TRUE(duration.count() < 1000);
}

/**
 * Test defer with complex cleanup scenario
 */
TEST(defer_complex_cleanup) {
    tang::RuntimeScope runtime(1);
    
    struct TestResource {
        std::atomic_int* cleanup_counter;
        int id;
        
        TestResource(std::atomic_int* counter, int resource_id) 
            : cleanup_counter(counter), id(resource_id) {
            LOG_INFO(logger::test) << "Resource " << id << " created";
        }
        
        ~TestResource() {
            (*cleanup_counter)++;
            LOG_INFO(logger::test) << "Resource " << id << " destroyed, cleanup count: " 
                      << cleanup_counter->load();
        }
    };
    
    std::atomic_int cleanup_count{0};
    std::atomic_bool main_logic_completed{false};
    
    tang::go([&cleanup_count, &main_logic_completed]() {
        LOG_INFO(logger::test) << "Complex cleanup test starting";
        
        // Create resources that need cleanup
        TestResource res1(&cleanup_count, 1);
        TestResource res2(&cleanup_count, 2);
        
        defer {
            // Cleanup operations
            LOG_INFO(logger::test) << "Defer cleanup executing";
        } end_defer
        
        // Simulate some work
        LOG_INFO(logger::test) << "Working with resources...";
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        main_logic_completed = true;
        
        LOG_INFO(logger::test) << "Main logic completed";
    });
    
    runtime.run();
    
    ASSERT_TRUE(main_logic_completed.load());
    ASSERT_EQUAL(2, cleanup_count.load()); // Both resources should be cleaned up
    LOG_INFO(logger::test) << "Complex cleanup defer test passed";
}

/**
 * Main function
 */
int main(int argc, char* argv[]) {
    return tang::test::run_tests(argc, argv);
}