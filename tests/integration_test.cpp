#include "test_framework.h"
#include <tang/tang.h>
#include <atomic>
#include <thread>
#include <vector>
#include <queue>
#include <functional>

/**
 * Test producer-consumer pattern
 */
TEST(producer_consumer_pattern) {
    tang::test::RuntimeScope runtime(4);
    
    const int num_producers = 3;
    const int num_consumers = 2;
    const int items_per_producer = 50;
    tang::channel<int> ch(20);
    std::atomic_int total_produced{0};
    std::atomic_int total_consumed{0};
    
    for (int p = 0; p < num_producers; ++p) {
        tang::go([&ch, &total_produced, p, items_per_producer]() {
            for (int i = 0; i < items_per_producer; ++i) {
                int value = p * 100 + i;
                ch << value;
                total_produced++;
            }
        });
    }
    
    for (int c = 0; c < num_consumers; ++c) {
        tang::go([&ch, &total_consumed, num_producers, items_per_producer]() {
            int value;
            int consumed = 0;
            int total_expected = num_producers * items_per_producer;
            
            while (consumed < total_expected && (ch >> value)) {
                consumed++;
                total_consumed++;
            }
        });
    }
    
    runtime.run();
    
    ASSERT_EQUAL(num_producers * items_per_producer, total_produced.load());
    ASSERT_EQUAL(num_producers * items_per_producer, total_consumed.load());
}

/**
 * Test pipeline pattern
 */
TEST(pipeline_pattern) {
    tang::test::RuntimeScope runtime(4);
    
    const int num_items = 20;
    
    // Use separate channel variables to avoid using vector
    tang::channel<int> stage0(10);
    tang::channel<int> stage1(10);
    tang::channel<int> stage2(10);
    tang::channel<int> stage3(10);
    
    std::atomic_int processed_count{0};
    
    tang::go([&stage0, num_items]() {
        for (int i = 0; i < num_items; ++i) {
            stage0 << i;
        }
        stage0.close();
    });
    
    tang::go([&stage0, &stage1, &processed_count, num_items]() {
        int value;
        int count = 0;
        while (count < num_items && (stage0 >> value)) {
            stage1 << value * 1;
            count++;
        }
        stage1.close();
    });
    
    tang::go([&stage1, &stage2, &processed_count, num_items]() {
        int value;
        int count = 0;
        while (count < num_items && (stage1 >> value)) {
            stage2 << value * 2;
            count++;
        }
        stage2.close();
    });
    
    tang::go([&stage2, &stage3, &processed_count, num_items]() {
        int value;
        int count = 0;
        while (count < num_items && (stage2 >> value)) {
            stage3 << value * 3;
            count++;
        }
        stage3.close();
    });
    
    tang::go([&stage3, &processed_count, num_items]() {
        int value;
        int count = 0;
        while (count < num_items && (stage3 >> value)) {
            processed_count++;
            count++;
        }
    });
    
    runtime.run();
    
    ASSERT_EQUAL(num_items, processed_count.load());
}

/**
 * Test work stealing pattern
 */
TEST(work_stealing_pattern) {
    tang::test::RuntimeScope runtime(4);
    
    const int num_tasks = 40;
    tang::channel<int> task_queue(20);
    std::atomic_int completed_tasks{0};
    std::atomic_int total_work{0};
    
    // Add tasks to queue
    tang::go([&task_queue, num_tasks]() {
        for (int i = 0; i < num_tasks; ++i) {
            task_queue << i;
        }
        task_queue.close();
    });
    
    // Create worker coroutines
    for (int w = 0; w < 4; ++w) {
        tang::go([&task_queue, &completed_tasks, &total_work]() {
            int task;
            while (task_queue >> task) {
                // Simulate some work
                total_work += task;
                completed_tasks++;
            }
        });
    }
    
    runtime.run();
    
    ASSERT_EQUAL(num_tasks, completed_tasks.load());
    // Verify total work (sum of 0..39 = 780)
    ASSERT_EQUAL(780, total_work.load());
}

/**
 * Test fan-out fan-in pattern
 */
TEST(fan_out_fan_in_pattern) {
    tang::test::RuntimeScope runtime(4);
    
    const int num_inputs = 30;
    const int num_workers = 3;
    tang::channel<int> input_channel(10);
    tang::channel<int> output_channel(10);
    std::atomic_int processed_count{0};
    
    // Producer: generate inputs
    tang::go([&input_channel, num_inputs]() {
        for (int i = 0; i < num_inputs; ++i) {
            input_channel << i;
        }
        input_channel.close();
    });
    
    // Workers: process inputs (fan-out)
    for (int w = 0; w < num_workers; ++w) {
        tang::go([&input_channel, &output_channel, w]() {
            int input;
            while (input_channel >> input) {
                // Process input: square the number
                int result = input * input;
                output_channel << result;
            }
        });
    }
    
    // Collector: gather results (fan-in)
    tang::go([&output_channel, &processed_count, num_inputs]() {
        int result;
        int count = 0;
        while (count < num_inputs && (output_channel >> result)) {
            processed_count++;
            count++;
        }
    });
    
    runtime.run();
    
    ASSERT_EQUAL(num_inputs, processed_count.load());
}

/**
 * Test mixed patterns integration
 */
TEST(mixed_patterns_integration) {
    tang::test::RuntimeScope runtime(6);
    
    const int num_items = 25;
    tang::channel<int> data_channel(15);
    tang::channel<int> result_channel(15);
    std::atomic_int final_result{0};
    
    // Pattern 1: Producer
    tang::go([&data_channel, num_items]() {
        for (int i = 0; i < num_items; ++i) {
            data_channel << i;
        }
        data_channel.close();
    });
    
    // Pattern 2: Pipeline with multiple stages
    tang::channel<int> stage1(10);
    tang::channel<int> stage2(10);
    
    // Stage 1: Multiply by 2
    tang::go([&data_channel, &stage1]() {
        int value;
        while (data_channel >> value) {
            stage1 << value * 2;
        }
        stage1.close();
    });
    
    // Stage 2: Add 10
    tang::go([&stage1, &stage2]() {
        int value;
        while (stage1 >> value) {
            stage2 << value + 10;
        }
        stage2.close();
    });
    
    // Pattern 3: Fan-out to multiple workers
    for (int w = 0; w < 3; ++w) {
        tang::go([&stage2, &result_channel, w]() {
            int value;
            while (stage2 >> value) {
                // Each worker processes differently
                int result = value + w * 5;
                result_channel << result;
            }
        });
    }
    
    // Pattern 4: Fan-in collector
    tang::go([&result_channel, &final_result, num_items]() {
        int result;
        int total = 0;
        int count = 0;
        while (count < num_items && (result_channel >> result)) {
            total += result;
            count++;
        }
        final_result = total;
    });
    
    runtime.run();
    
    // Verify final result
    ASSERT_TRUE(final_result.load() > 0);
}

/**
 * Test error handling in integration scenarios
 */
TEST(integration_error_handling) {
    tang::test::RuntimeScope runtime(3);
    
    tang::channel<int> ch(5);
    std::atomic_int successful_operations{0};
    std::atomic_int error_count{0};
    
    // Producer with potential errors
    tang::go([&ch, &error_count]() {
        try {
            for (int i = 0; i < 10; ++i) {
                if (i == 5) {
                    throw std::runtime_error("Simulated error");
                }
                ch << i;
            }
        } catch (const std::exception&) {
            error_count++;
        }
        ch.close();
    });
    
    // Consumer that handles errors gracefully
    tang::go([&ch, &successful_operations]() {
        int value;
        while (ch >> value) {
            successful_operations++;
        }
    });
    
    runtime.run();
    
    // Should have processed items before error
    ASSERT_EQUAL(5, successful_operations.load());
    ASSERT_EQUAL(1, error_count.load());
}

/**
 * Test resource cleanup in integration scenarios
 */
TEST(integration_resource_cleanup) {
    // Test multiple runtime cycles
    for (int cycle = 0; cycle < 3; ++cycle) {
        tang::test::RuntimeScope runtime(2);
        
        tang::channel<int> ch(5);
        std::atomic_int processed{0};
        
        tang::go([&ch]() {
            for (int i = 0; i < 5; ++i) {
                ch << i;
            }
            ch.close();
        });
        
        tang::go([&ch, &processed]() {
            int value;
            while (ch >> value) {
                processed++;
            }
        });
        
        runtime.run();
        
        // Each cycle should process all items
        ASSERT_EQUAL(5, processed.load());
    }
}

/**
 * Main function using test framework
 */
int main(int argc, char* argv[]) {
    return tang::test::run_tests(argc, argv);
}