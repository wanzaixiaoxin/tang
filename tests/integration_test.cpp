#include <iostream>
#include <cassert>
#include <tang/tang.h>
#include <vector>
#include <atomic>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <queue>
#include <functional>

#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion failed: " << #condition << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::terminate(); \
        } \
    } while(0)

void test_producer_consumer_pattern() {
    std::cout << "测试生产者-消费者模式..." << std::endl;
    const int num_producers = 3;
    const int num_consumers = 2;
    const int items_per_producer = 50;
    tang::channel<int> ch(20);
    std::atomic_int total_produced{0};
    std::atomic_int total_consumed{0};
    
    tang::runtime::init(4);
    
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
    
    tang::runtime::run();
    
    ASSERT(total_produced.load() == num_producers * items_per_producer);
    ASSERT(total_consumed.load() == num_producers * items_per_producer);
    tang::runtime::stop();
    std::cout << "生产者-消费者模式测试通过!" << std::endl;
}

void test_pipeline_pattern() {
    std::cout << "测试管道模式..." << std::endl;
    const int num_items = 20;
    
    // 使用独立的channel变量，避免使用vector
    tang::channel<int> stage0(10);
    tang::channel<int> stage1(10);
    tang::channel<int> stage2(10);
    tang::channel<int> stage3(10);
    
    std::atomic_int processed_count{0};
    
    tang::runtime::init(4);
    
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
        processed_count = count;
        stage3.close();
    });
    
    tang::runtime::run();
    
    ASSERT(processed_count.load() == num_items);
    tang::runtime::stop();
    std::cout << "管道模式测试通过!" << std::endl;
}

void test_fan_out_pattern() {
    std::cout << "测试扇出模式..." << std::endl;
    const int num_workers = 4;
    const int items_per_worker = 25;
    tang::channel<int> input(50);
    
    // 使用独立的channel变量，避免使用vector
    tang::channel<int> worker0(10);
    tang::channel<int> worker1(10);
    tang::channel<int> worker2(10);
    tang::channel<int> worker3(10);
    
    std::atomic_int total_distributed{0};
    std::atomic_int total_processed{0};
    
    tang::runtime::init(4);
    
    tang::go([&input, &total_distributed, num_workers, items_per_worker]() {
        for (int i = 0; i < num_workers * items_per_worker; ++i) {
            input << i;
            total_distributed++;
        }
        input.close();
    });
    
    tang::go([&input, &worker0, &total_processed, items_per_worker]() {
        int count = 0;
        int value;
        while (count < items_per_worker && (input >> value)) {
            worker0 << value * 2;
            count++;
            total_processed++;
        }
    });
    
    tang::go([&input, &worker1, &total_processed, items_per_worker]() {
        int count = 0;
        int value;
        while (count < items_per_worker && (input >> value)) {
            worker1 << value * 2;
            count++;
            total_processed++;
        }
    });
    
    tang::go([&input, &worker2, &total_processed, items_per_worker]() {
        int count = 0;
        int value;
        while (count < items_per_worker && (input >> value)) {
            worker2 << value * 2;
            count++;
            total_processed++;
        }
    });
    
    tang::go([&input, &worker3, &total_processed, items_per_worker]() {
        int count = 0;
        int value;
        while (count < items_per_worker && (input >> value)) {
            worker3 << value * 2;
            count++;
            total_processed++;
        }
    });
    
    tang::runtime::run();
    
    ASSERT(total_distributed.load() == num_workers * items_per_worker);
    ASSERT(total_processed.load() == num_workers * items_per_worker);
    tang::runtime::stop();
    std::cout << "扇出模式测试通过!" << std::endl;
}

void test_fan_in_pattern() {
    std::cout << "测试扇入模式..." << std::endl;
    const int num_sources = 4;
    const int items_per_source = 25;
    tang::channel<int> ch0, ch1, ch2, ch3;
    tang::channel<int> merged(50);
    std::atomic_int total_sent{0};
    std::atomic_int total_received{0};
    
    tang::runtime::init(4);
    
    tang::go([&ch0, &total_sent, items_per_source]() {
        for (int i = 0; i < items_per_source; ++i) {
            ch0 << i;
            total_sent++;
        }
        ch0.close();
    });
    
    tang::go([&ch1, &total_sent, items_per_source]() {
        for (int i = 0; i < items_per_source; ++i) {
            ch1 << (100 + i);
            total_sent++;
        }
        ch1.close();
    });
    
    tang::go([&ch2, &total_sent, items_per_source]() {
        for (int i = 0; i < items_per_source; ++i) {
            ch2 << (200 + i);
            total_sent++;
        }
        ch2.close();
    });
    
    tang::go([&ch3, &total_sent, items_per_source]() {
        for (int i = 0; i < items_per_source; ++i) {
            ch3 << (300 + i);
            total_sent++;
        }
        ch3.close();
    });
    
    tang::go([&ch0, &ch1, &ch2, &ch3, &merged, &total_received, num_sources, items_per_source]() {
        int received_count = 0;
        int total_expected = num_sources * items_per_source;
        int value = 0;
        
        while (received_count < total_expected) {
            tang::select(
                tang::case_recv(ch0, value, [&]() {}),
                tang::case_recv(ch1, value, [&]() {}),
                tang::case_recv(ch2, value, [&]() {}),
                tang::case_recv(ch3, value, [&]() {}),
                tang::default_case([&]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                })
            );
            
            if (ch0.try_recv(value)) {
                merged << value;
                total_received++;
                received_count++;
            } else if (ch1.try_recv(value)) {
                merged << value;
                total_received++;
                received_count++;
            } else if (ch2.try_recv(value)) {
                merged << value;
                total_received++;
                received_count++;
            } else if (ch3.try_recv(value)) {
                merged << value;
                total_received++;
                received_count++;
            }
        }
    });
    
    tang::go([&merged, &total_received, num_sources, items_per_source]() {
        int value;
        int count = 0;
        int total_expected = num_sources * items_per_source;
        while (count < total_expected && (merged >> value)) {
            count++;
        }
    });
    
    tang::runtime::run();
    
    ASSERT(total_sent.load() == num_sources * items_per_source);
    ASSERT(total_received.load() == num_sources * items_per_source);
    tang::runtime::stop();
    std::cout << "扇入模式测试通过!" << std::endl;
}

void test_parallel_computation() {
    std::cout << "测试并行计算..." << std::endl;
    const int num_tasks = 10;
    const int data_size = 1000;
    std::atomic<long long> total_sum{0};
    
    tang::runtime::init(4);
    
    for (int task = 0; task < num_tasks; ++task) {
        tang::go([&total_sum, task, data_size]() {
            long long local_sum = 0;
            for (int i = task * data_size; i < (task + 1) * data_size; ++i) {
                local_sum += i;
            }
            total_sum += local_sum;
        });
    }
    
    tang::runtime::run();
    
    long long expected = 0;
    for (int i = 0; i < num_tasks * data_size; ++i) {
        expected += i;
    }
    ASSERT(total_sum.load() == expected);
    tang::runtime::stop();
    std::cout << "并行计算测试通过!" << std::endl;
}

void test_concurrent_accumulation() {
    std::cout << "测试并发累加..." << std::endl;
    const int num_workers = 8;
    const int iterations = 10000;
    std::atomic<long long> counter{0};
    
    tang::runtime::init(4);
    
    for (int w = 0; w < num_workers; ++w) {
        tang::go([&counter, iterations]() {
            for (int i = 0; i < iterations; ++i) {
                counter++;
            }
        });
    }
    
    tang::runtime::run();
    
    ASSERT(counter.load() == num_workers * iterations);
    tang::runtime::stop();
    std::cout << "并发累加测试通过!" << std::endl;
}

void test_barrier_pattern() {
    std::cout << "测试屏障模式..." << std::endl;
    const int num_workers = 4;
    std::atomic_int ready_count{0};
    std::atomic_int completed_count{0};
    tang::channel<int> start_ch;
    tang::channel<int> done_ch;
    
    tang::runtime::init(4);
    
    for (int w = 0; w < num_workers; ++w) {
        tang::go([&ready_count, &completed_count, &start_ch, &done_ch, w]() {
            ready_count++;
            
            // 等待所有worker准备就绪
            int dummy;
            start_ch >> dummy;
            
            completed_count++;
            done_ch << w;
        });
    }
    
    // 等待所有worker准备就绪
    while (ready_count.load() < num_workers) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // 通知所有worker开始
    for (int w = 0; w < num_workers; ++w) {
        start_ch << 1;
    }
    
    tang::runtime::run();
    
    ASSERT(completed_count.load() == num_workers);
    tang::runtime::stop();
    std::cout << "屏障模式测试通过!" << std::endl;
}

void test_broadcast_pattern() {
    std::cout << "测试广播模式..." << std::endl;
    const int num_listeners = 3;
    const int messages = 5;
    std::atomic_int total_received{0};
    
    tang::runtime::init(4);
    
    tang::channel<int> broadcast_ch;
    
    for (int l = 0; l < num_listeners; ++l) {
        tang::go([&broadcast_ch, &total_received, messages, l]() {
            int value;
            for (int m = 0; m < messages; ++m) {
                broadcast_ch >> value;
                total_received++;
            }
        });
    }
    
    tang::go([&broadcast_ch, messages, num_listeners]() {
        for (int m = 0; m < messages; ++m) {
            for (int l = 0; l < num_listeners; ++l) {
                broadcast_ch << m;
            }
        }
    });
    
    tang::runtime::run();
    
    ASSERT(total_received.load() == messages * num_listeners);
    tang::runtime::stop();
    std::cout << "广播模式测试通过!" << std::endl;
}

void test_work_stealing_simulation() {
    std::cout << "测试工作窃取模拟..." << std::endl;
    const int num_workers = 4;
    const int tasks_per_thread = 20;
    std::vector<std::queue<int>> local_queues(num_workers);
    std::queue<int> steal_queue;
    std::atomic_int total_completed{0};
    
    tang::runtime::init(4);
    
    for (int t = 0; t < num_workers; ++t) {
        for (int i = 0; i < tasks_per_thread; ++i) {
            local_queues[t].push(t * 100 + i);
        }
    }
    
    for (int t = 0; t < num_workers; ++t) {
        tang::go([&local_queues, &steal_queue, &total_completed, t, tasks_per_thread, num_workers]() {
            int completed = 0;
            
            while (completed < tasks_per_thread) {
                int task_id = -1;
                
                // 尝试从本地队列获取任务
                if (!local_queues[t].empty()) {
                    task_id = local_queues[t].front();
                    local_queues[t].pop();
                } else {
                    // 尝试窃取其他队列的任务
                    for (int other = 0; other < num_workers; ++other) {
                        if (other != t && !local_queues[other].empty()) {
                            task_id = local_queues[other].front();
                            local_queues[other].pop();
                            break;
                        }
                    }
                }
                
                if (task_id != -1) {
                    // 模拟任务执行
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    completed++;
                    total_completed++;
                } else {
                    // 没有任务可执行，让出CPU
                    tang::runtime::yield();
                }
            }
        });
    }
    
    tang::runtime::run();
    
    ASSERT(total_completed.load() == num_workers * tasks_per_thread);
    tang::runtime::stop();
    std::cout << "工作窃取模拟测试通过!" << std::endl;
}

void test_cancellation_simulation() {
    std::cout << "测试取消模拟..." << std::endl;
    const int num_tasks = 5;
    std::atomic<bool> cancel_flag{false};
    std::atomic_int completed_tasks{0};
    
    tang::runtime::init(4);
    
    for (int t = 0; t < num_tasks; ++t) {
        tang::go([&cancel_flag, &completed_tasks, t]() {
            int iterations = 0;
            while (!cancel_flag.load() && iterations < 100) {
                // 模拟长时间运行的任务
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                iterations++;
            }
            completed_tasks++;
        });
    }
    
    // 运行一段时间后取消
    tang::go([&cancel_flag]() {
        tang::runtime::sleep_ms(50);
        cancel_flag = true;
    });
    
    tang::runtime::run();
    
    ASSERT(completed_tasks.load() == num_tasks);
    ASSERT(cancel_flag.load() == true);
    tang::runtime::stop();
    std::cout << "取消模拟测试通过!" << std::endl;
}

int main() {
    std::cout << "开始运行集成测试..." << std::endl;
    
    try {
        test_producer_consumer_pattern();
        test_pipeline_pattern();
        test_fan_out_pattern();
        test_fan_in_pattern();
        test_parallel_computation();
        test_concurrent_accumulation();
        test_barrier_pattern();
        test_broadcast_pattern();
        test_work_stealing_simulation();
        test_cancellation_simulation();
        
        std::cout << "\n所有集成测试通过!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "测试失败: " << e.what() << std::endl;
        return 1;
    }
}
