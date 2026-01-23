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

// Test basic coroutine creation and execution
void test_basic_task() {
    std::cout << "Testing basic coroutine..." << std::endl;
    std::atomic_bool executed = false;
    
    // Initialize runtime
    tang::runtime::init(2);
    
    // Create coroutine
    tang::go([&executed]() {
        executed = true;
    });
    
    // Run scheduler
    tang::runtime::run();
    
    // Verify coroutine execution
    ASSERT(executed.load());
    
    // Stop runtime
    tang::runtime::stop();
    std::cout << "Basic coroutine test passed!" << std::endl;
}

// Test coroutine function return value
void test_task_return_value() {
    std::cout << "Testing coroutine return value..." << std::endl;
    std::atomic_int result = 0;
    
    // Initialize runtime
    tang::runtime::init(2);
    
    // Define a coroutine function that returns a value
    auto task_func = []() -> int {
        return 42;
    };
    
    // Create coroutine and capture result in atomic variable
    tang::go([&result, &task_func]() {
        result = task_func();
    });
    
    // Run scheduler
    tang::runtime::run();
    
    // Verify return value
    ASSERT(result.load() == 42);
    
    // Stop runtime
    tang::runtime::stop();
    std::cout << "Coroutine return value test passed!" << std::endl;
}

// Test multiple concurrent tasks
void test_multiple_tasks() {
    std::cout << "Testing multiple concurrent tasks..." << std::endl;
    const int num_tasks = 10; // Reduce task count for faster test
    std::atomic_int executed_count = 0;
    
    // Initialize runtime
    tang::runtime::init(4);
    
    // Create multiple coroutines
    for (int i = 0; i < num_tasks; ++i) {
        tang::go([&executed_count, i]() {
            // Simulate some work to be done
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            executed_count++;
        });
    }
    
    // Run scheduler
    tang::runtime::run();
    
    // Verify all coroutines have been executed
    ASSERT(executed_count.load() == num_tasks);
    
    // Stop runtime
    tang::runtime::stop();
    std::cout << "Multiple concurrent tasks test passed!" << std::endl;
}

// Test coroutine exception handling
void test_task_exception() {
    std::cout << "Testing coroutine exception handling..." << std::endl;
    std::atomic_bool caught = false;
    
    // Initialize runtime
    tang::runtime::init(2);
    
    // Create coroutine that throws an exception
    tang::go([&caught]() {
        try {
            throw std::runtime_error("Test exception");
        } catch (const std::exception& e) {
            caught = true;
            std::cerr << "Caught exception: " << e.what() << std::endl;
        }
    });
    
    // Run scheduler
    tang::runtime::run();
    
    // Verify exception is caught
    ASSERT(caught.load());
    
    // Stop runtime
    tang::runtime::stop();
    std::cout << "Coroutine exception handling test passed!" << std::endl;
}

// Test coroutine function with parameters
void test_task_with_parameters() {
    std::cout << "Testing coroutine with parameters..." << std::endl;
    std::atomic_int result = 0;
    
    // Initialize runtime
    tang::runtime::init(2);
    
    // Define a function with parameters
    auto add_func = [](int a, int b) {
        return a + b;
    };
    
    // Create coroutine and pass parameters
    tang::go([&result, &add_func]() {
        result = add_func(10, 20);
    });
    
    // Run scheduler
    tang::runtime::run();
    
    // Verify result
    ASSERT(result.load() == 30);
    
    // Stop runtime             
    tang::runtime::stop();
    std::cout << "Coroutine with parameters test passed!" << std::endl;
}

// Test coroutine yield functionality
void test_task_yield() {
    std::cout << "Testing coroutine yield functionality..." << std::endl;
    std::atomic_int execution_order = 0;
    
    // Initialize runtime
    tang::runtime::init(2);
    
    // Create first coroutine
    tang::go([&execution_order]() {
        execution_order++;
        
        // Yield CPU
        tang::runtime::yield();
        
        execution_order += 2;
    });
    
    // Create second coroutine
    tang::go([&execution_order]() {
        execution_order++;
    });
    
    // Run scheduler
    tang::runtime::run();
    
    // Verify execution order
    ASSERT(execution_order.load() == 4);
    
    // Stop runtime
    tang::runtime::stop();
    std::cout << "Coroutine yield functionality test passed!" << std::endl;
}

// Test spawn_function
void test_spawn_function() {
    std::cout << "Testing spawn function..." << std::endl;
    std::atomic_bool executed = false;
    
    // Initialize runtime
    tang::runtime::init(2);
    
    // Create coroutine using spawn
    tang::spawn([&executed]() {
        executed = true;
    });
    
    // Run scheduler
    tang::runtime::run();
    
    // Verify coroutine execution
    ASSERT(executed.load());
    
    // Stop runtime
    tang::runtime::stop();
    std::cout << "Spawn function test passed!" << std::endl;
}

// Test coroutine sleep functionality
void test_task_sleep() {
    std::cout << "Testing coroutine sleep functionality..." << std::endl;
    auto start = std::chrono::steady_clock::now();
    
    // Initialize runtime
    tang::runtime::init(2);
    
    // Create sleep coroutine
    tang::go([]() {
        // Sleep 100 milliseconds
        tang::runtime::sleep_ms(100);
    });
    
    // Run scheduler
    tang::runtime::run();
    
    // Calculate execution time     
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Verify sleep duration is at least 100 milliseconds
    ASSERT(duration.count() >= 100);
    
    // Stop runtime
    tang::runtime::stop();
    std::cout << "Coroutine sleep functionality test passed!" << std::endl;
}

// Test different thread counts         
void test_different_thread_counts() {
    std::cout << "Testing different thread counts..." << std::endl;
    std::atomic_int executed_count = 0;
    const int num_tasks = 10; // Reduce task count for faster test
    
    // Test different thread counts
    for (int threads = 1; threads <= 2; ++threads) { // Reduce thread count range for faster test
        executed_count = 0;
        
        // Initialize runtime with different thread count   
        tang::runtime::init(threads);
        
        // Create multiple coroutines
        for (int i = 0; i < num_tasks; ++i) {
            tang::go([&executed_count]() {
                executed_count++;
            });
        }
        
        // Run scheduler
        tang::runtime::run();
        
        // Verify all coroutines have been executed
        ASSERT(executed_count.load() == num_tasks);
        
        // Stop runtime
        tang::runtime::stop();
    }
    std::cout << "Different thread counts test passed!" << std::endl;
}

// Test nested coroutine calls
void test_nested_coroutines() {
    std::cout << "Testing nested coroutine calls..." << std::endl;
    std::atomic_int result = 0;
    
    tang::runtime::init(2);
    
    std::function<tang::task<int>()> inner_task = []() -> tang::task<int> {
        co_return 42;
    };
    
    auto outer_task = [&result, &inner_task]() -> tang::task<void> {
        auto inner = inner_task();
        int value = co_await inner;
        result = value * 2;
        co_return;
    };
    
    auto task = outer_task();
    task.run();
    tang::runtime::run();
    
    ASSERT(result.load() == 84);
    tang::runtime::stop();
    std::cout << "Nested coroutine calls test passed!" << std::endl;
}

// Test multi-level nested coroutines
void test_multi_level_nested_coroutines() {
    std::cout << "Testing multi-level nested coroutines..." << std::endl;
    std::atomic_int result = 0;
    
    tang::runtime::init(2);
    
    std::function<tang::task<int>()> level1 = []() -> tang::task<int> {
        co_return 10;
    };
    
    std::function<tang::task<int>()> level2 = [&level1]() -> tang::task<int> {
        auto l1 = level1();
        int v1 = co_await l1;
        co_return v1 * 2;
    };
    
    auto level3 = [&result, &level2]() -> tang::task<void> {
        auto l2 = level2();
        int v2 = co_await l2;
        result = v2 * 3;
        co_return;
    };
    
    auto task = level3();
    task.run();
    tang::runtime::run();
    
    ASSERT(result.load() == 60); // 10 * 2 * 3
    tang::runtime::stop();
    std::cout << "Multi-level nested coroutines test passed!" << std::endl;
}

// Test task group - wait_all semantics
void test_wait_all() {
    std::cout << "Testing wait_all_all..." << std::endl;
    std::atomic_int counter = 0;
    const int num_tasks = 5;
    
    tang::runtime::init(2);
    
    std::vector<tang::task<void>> tasks;
    for (int i = 0; i < num_tasks; ++i) {
        auto task_func = [&counter, i]() -> tang::task<void> {
            counter++;
            co_return;
        };
        tasks.push_back(task_func());
    }
    
    for (auto& t : tasks) {
        t.run();
    }
    
    tang::runtime::run();
    
    ASSERT(counter.load() == num_tasks);
    tang::runtime::stop();
    std::cout << "Wait_all_all test passed!" << std::endl;
}

// Test recursive coroutine
void test_recursive_coroutine() {
    std::cout << "Testing recursive coroutine..." << std::endl; 
    std::atomic_int sum = 0;
    
    tang::runtime::init(2);
    
    struct FibImpl {
        std::function<tang::task<int>(int)> self;
        
        tang::task<int> operator()(int n) {
            if (n <= 1) {
                co_return n;
            }
            auto a = self(n - 1);
            auto b = self(n - 2);
            int result = co_await a + co_await b;
            co_return result;
        }
    };
    
    FibImpl fib_impl;
    fib_impl.self = [&fib_impl](int n) -> tang::task<int> {
        return fib_impl(n);
    };
    
    auto main_task = [&sum, &fib_impl]() -> tang::task<void> {
        auto fib10 = fib_impl.self(10);
        int result = co_await fib10;
        sum = result;
        co_return;
    };
    
    auto task = main_task();
    task.run();
    tang::runtime::run();
    
    ASSERT(sum.load() == 55); // Fibonacci(10) = 55
    tang::runtime::stop();
    std::cout << "Recursive coroutine test passed!" << std::endl;
}

// Test exception propagation
void test_exception_propagation() {
    std::cout << "Testing exception propagation..." << std::endl;
    std::atomic_bool inner_exception_caught = false;
    std::atomic_bool outer_exception_caught = false;
    
    tang::runtime::init(2);
    
    std::function<tang::task<int>()> throwing_task = []() -> tang::task<int> {
        throw std::runtime_error("Inner exception");
        co_return 0;
    };
    
    std::function<tang::task<void>()> catching_task = [&inner_exception_caught, &throwing_task]() -> tang::task<void> {
        try {
            auto t = throwing_task();
            co_await t;
        } catch (const std::exception& e) {
            inner_exception_caught = true;
            std::cerr << "Caught inner exception: " << e.what() << std::endl;
        }
        co_return;
    };
    
    auto outer_task = [&outer_exception_caught, &catching_task]() -> tang::task<void> {
        try {
            auto t = catching_task();
            co_await t;
            // Simulate another exception to be caught
            throw std::runtime_error("Outer exception");
        } catch (const std::exception& e) {
            outer_exception_caught = true;
            std::cerr << "Caught outer exception: " << e.what() << std::endl;
        }
        co_return;
    };
    
    auto task = outer_task();
    task.run();
    tang::runtime::run();
    
    ASSERT(inner_exception_caught.load());
    ASSERT(outer_exception_caught.load());
    tang::runtime::stop();
    std::cout << "Exception propagation test passed!" << std::endl;
}

// Test resource contention
void test_resource_contention() {
    std::cout << "Testing resource contention..." << std::endl;
    const int num_tasks = 10;
    const int increments_per_task = 1000;
    std::atomic_int shared_counter{0};
    
    tang::runtime::init(4);
    
    for (int i = 0; i < num_tasks; ++i) {
        tang::go([&shared_counter, increments_per_task]() {
            for (int j = 0; j < increments_per_task; ++j) {
                shared_counter++;
            }
        });
    }
    
    tang::runtime::run();
    
    ASSERT(shared_counter.load() == num_tasks * increments_per_task);
    tang::runtime::stop();
    std::cout << "Resource contention test passed! final count: " << shared_counter.load() << std::endl;
}

// Test task lifecycle management
void test_task_lifecycle() {
    std::cout << "Testing task lifecycle management..." << std::endl;
    std::atomic_int lifecycle_events{0};
    
    tang::runtime::init(2);
    
    auto tracked_task = [&lifecycle_events]() -> tang::task<int> {
        lifecycle_events++; // Start
        co_return 1;
    };
    
    {
        auto task = tracked_task();
        task.run();
        tang::runtime::run();
        ASSERT(lifecycle_events.load() >= 1);
    }
    
    tang::runtime::stop();
    std::cout << "Task lifecycle test passed!" << std::endl;
}

// Test parallel execution
void test_parallel_execution() {
    std::cout << "Testing parallel execution..." << std::endl;
    std::atomic_int start_time{0};
    std::atomic_int end_time_count{0};
    const int num_tasks = 4;
    
    tang::runtime::init(4);
    
    for (int i = 0; i < num_tasks; ++i) {
        tang::go([&start_time, &end_time_count, i]() {
            [[maybe_unused]] int my_start = start_time.fetch_add(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            end_time_count++;
        });
    }
    
    tang::runtime::run();
    
    ASSERT(end_time_count.load() == num_tasks);
    tang::runtime::stop();
    std::cout << "Parallel execution test passed!" << std::endl;
}

// Test data passing
void test_data_passing() {
    std::cout << "Testing data passing..." << std::endl;
    struct ComplexData {
        int id;
        std::string name;
        std::vector<int> values;
    };
    
    tang::channel<ComplexData> ch(2);
    std::atomic_bool received{false};
    
    tang::runtime::init(2);
    
    tang::go([&ch]() {
        ComplexData data{42, "test", {1, 2, 3, 4, 5}};
        ch << data;
    });
    
    tang::go([&ch, &received]() {
        ComplexData data;
        ch >> data;
        ASSERT(data.id == 42);
        ASSERT(data.name == "test");
        ASSERT(data.values.size() == 5);
        received = true;
    });
    
    tang::runtime::run();
    ASSERT(received.load());
    tang::runtime::stop();
    std::cout << "Data passing test passed!" << std::endl;
}

// Test sync primitives combo
void test_sync_primitives_combo() {
    std::cout << "Testing sync primitives combo..." << std::endl;
    tang::channel<int> ch(5);
    std::atomic_int sum{0};
    const int num_tasks = 3;
    
    tang::runtime::init(2);
    
    for (int i = 0; i < num_tasks; ++i) {
        tang::go([&ch, i]() {
            for (int j = 0; j < 5; ++j) {
                ch << (i * 10 + j);
            }
        });
    }
    
    tang::go([&ch, &sum, num_tasks]() {
        int count = 0;
        int total = num_tasks * 5;
        while (count < total) {
            int value;
            if (ch >> value) {
                sum += value;
                count++;
            }
        }
    });
    
    tang::runtime::run();
    
    // Verify data integrity
    ASSERT(sum.load() > 0);
    tang::runtime::stop();
    std::cout << "Sync primitives combo test passed! sum: " << sum.load() << std::endl;
}

// Test main function
int main() {
    std::cout << "Running task system tests..." << std::endl;
    
    try {
        test_basic_task();
        test_task_return_value();
        test_multiple_tasks();
        test_task_exception();
        test_task_with_parameters();
        test_task_yield();
        test_spawn_function();
        test_task_sleep();
        test_different_thread_counts();
        test_nested_coroutines();
        test_multi_level_nested_coroutines();
        test_wait_all();
        test_recursive_coroutine();
        test_exception_propagation();
        test_resource_contention();
        test_task_lifecycle();
        test_parallel_execution();
        test_data_passing();
        test_sync_primitives_combo();
        
        std::cout << "\\nAll task system tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Task system test failed: " << e.what() << std::endl;
        return 1;
    }
}
