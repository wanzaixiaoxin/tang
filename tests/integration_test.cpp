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
    
    for (int i = 0; i < num_producers; ++i) {
        tang::go([&ch, &total_produced, i, items_per_producer]() {
            for (int j = 0; j < items_per_producer; ++j) {
                int value = i * 1000 + j;
                ch << value;
                total_produced++;
            }
        });
    }
    
    for (int i = 0; i < num_consumers; ++i) {
        tang::go([&ch, &total_consumed, total = num_producers * items_per_producer]() {
            int count = 0;
            while (count < total) {
                int value;
                if (ch >> value) {
                    total_consumed++;
                    count++;
                }
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
    const int pipeline_stages = 4;
    const int num_items = 20;
    
    std::vector<tang::channel<int>> stages(pipeline_stages);
    for (int i = 0; i < pipeline_stages; ++i) {
        stages[i] = tang::channel<int>(10);
    }
    
    std::atomic_int processed_count{0};
    
    tang::runtime::init(4);
    
    tang::go([&stages, num_items]() {
        for (int i = 0; i < num_items; ++i) {
            stages[0] << i;
        }
        stages[0].close();
    });
    
    for (int stage = 0; stage < pipeline_stages - 1; ++stage) {
        tang::go([&stages, stage, &processed_count, num_items]() {
            int value;
            int count = 0;
            while (count < num_items && (stages[stage] >> value)) {
                stages[stage + 1] << value * (stage + 1);
                count++;
            }
            if (stage == pipeline_stages - 2) {
                processed_count = count;
            }
            stages[stage + 1].close();
        });
    }
    
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
    std::vector<tang::channel<int>> worker_channels(num_workers);
    for (int i = 0; i < num_workers; ++i) {
        worker_channels[i] = tang::channel<int>(10);
    }
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
    
    for (int w = 0; w < num_workers; ++w) {
        tang::go([&input, &worker_channels, w, &total_processed, items_per_worker]() {
            int count = 0;
            int value;
            while (count < items_per_worker && (input >> value)) {
                worker_channels[w] << value * 2;
                count++;
                total_processed++;
            }
        });
    }
    
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
    const int num_workers = 4;
    const int chunk_size = 1000;
    tang::channel<int> chunks(10);
    tang::channel<long long> results(10);
    std::atomic_int chunks_sent{0};
    std::atomic_long long total_sum{0};
    
    tang::runtime::init(4);
    
    tang::go([&chunks, &chunks_sent, num_workers, chunk_size]() {
        for (int i = 0; i < num_workers; ++i) {
            chunks << i;
            chunks_sent++;
        }
        chunks.close();
    });
    
    for (int w = 0; w < num_workers; ++w) {
        tang::go([&chunks, &results, w, chunk_size]() {
            int chunk_id;
            while (chunks >> chunk_id) {
                long long sum = 0;
                int start = chunk_id * chunk_size;
                int end = start + chunk_size;
                for (int i = start; i < end; ++i) {
                    sum += i;
                }
                results << sum;
            }
        });
    }
    
    tang::go([&results, &total_sum, num_workers]() {
        long long sum = 0;
        int count = 0;
        while (count < num_workers) {
            long long value;
            if (results >> value) {
                sum += value;
                count++;
            }
        }
        total_sum = sum;
    });
    
    tang::runtime::run();
    
    long long expected_sum = 0;
    for (int i = 0; i < num_workers * chunk_size; ++i) {
        expected_sum += i;
    }
    ASSERT(total_sum.load() == expected_sum);
    tang::runtime::stop();
    std::cout << "并行计算测试通过!" << std::endl;
}

void test_concurrent_accumulation() {
    std::cout << "测试并发累加..." << std::endl;
    const int num_accumulators = 4;
    const int increments_per_accumulator = 1000;
    tang::channel<int> channel(10);
    std::atomic_int total_increments{0};
    
    tang::runtime::init(4);
    
    for (int i = 0; i < num_accumulators; ++i) {
        tang::go([&channel, &total_increments, i, increments_per_accumulator]() {
            for (int j = 0; j < increments_per_accumulator; ++j) {
                channel << i;
                total_increments++;
            }
        });
    }
    
    std::atomic_int received_count{0};
    tang::go([&channel, &received_count, num_accumulators, increments_per_accumulator]() {
        int value;
        int count = 0;
        int total = num_accumulators * increments_per_accumulator;
        while (count < total) {
            if (channel >> value) {
                count++;
                received_count++;
            }
        }
    });
    
    tang::runtime::run();
    
    ASSERT(total_increments.load() == num_accumulators * increments_per_accumulator);
    ASSERT(received_count.load() == num_accumulators * increments_per_accumulator);
    tang::runtime::stop();
    std::cout << "并发累加测试通过!" << std::endl;
}

void test_barrier_pattern() {
    std::cout << "测试屏障模式..." << std::endl;
    const int num_workers = 4;
    const int phases = 3;
    tang::channel<int> barrier(10);
    std::atomic_int phase_completed{0};
    
    tang::runtime::init(4);
    
    for (int w = 0; w < num_workers; ++w) {
        tang::go([&barrier, &phase_completed, w, phases]() {
            for (int p = 0; p < phases; ++p) {
                barrier << w;
                if (w == 0) {
                    while (phase_completed.load() < num_workers - 1) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                    phase_completed = 0;
                } else {
                    phase_completed++;
                    int temp;
                    while (barrier >> temp) {}
                }
            }
        });
    }
    
    tang::runtime::run();
    tang::runtime::stop();
    std::cout << "屏障模式测试通过!" << std::endl;
}

void test_broadcast_pattern() {
    std::cout << "测试广播模式..." << std::endl;
    const int num_subscribers = 4;
    std::vector<tang::channel<int>> subscriber_channels(num_subscribers);
    std::atomic_int messages_delivered{0};
    
    tang::runtime::init(4);
    
    tang::go([&subscriber_channels]() {
        for (int msg = 0; msg < 10; ++msg) {
            for (int s = 0; s < num_subscribers; ++s) {
                subscriber_channels[s] << msg;
            }
        }
        for (int s = 0; s < num_subscribers; ++s) {
            subscriber_channels[s].close();
        }
    });
    
    for (int s = 0; s < num_subscribers; ++s) {
        tang::go([&subscriber_channels, s, &messages_delivered]() {
            int value;
            while (subscriber_channels[s] >> value) {
                messages_delivered++;
            }
        });
    }
    
    tang::runtime::run();
    
    ASSERT(messages_delivered.load() == 10 * num_subscribers);
    tang::runtime::stop();
    std::cout << "广播模式测试通过!" << std::endl;
}

void test_work_stealing_simulation() {
    std::cout << "测试工作窃取模拟..." << std::endl;
    const int num_threads = 4;
    const int tasks_per_thread = 10;
    std::vector<tang::channel<int>> local_queues(num_threads);
    tang::channel<int> steal_queue(50);
    std::atomic_int total_completed{0};
    int thread_id_counter = 0;
    int my_thread_id = 0;
    
    tang::runtime::init(num_threads);
    
    for (int t = 0; t < num_threads; ++t) {
        tang::go([&local_queues, &steal_queue, &total_completed, t, tasks_per_thread]() {
            my_thread_id = t;
            for (int i = 0; i < tasks_per_thread; ++i) {
                local_queues[t] << t * 100 + i;
            }
            
            int value;
            int local_count = 0;
            while (local_count < tasks_per_thread) {
                if (local_queues[t].try_recv(value)) {
                    total_completed++;
                    local_count++;
                } else {
                    bool stolen = false;
                    for (int other = 0; other < num_threads && !stolen; ++other) {
                        if (other != t) {
                            int steal_value;
                            if (local_queues[other].try_recv(steal_value)) {
                                total_completed++;
                                local_count++;
                                stolen = true;
                            }
                        }
                    }
                    if (!stolen) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                }
            }
        });
    }
    
    tang::runtime::run();
    
    ASSERT(total_completed.load() == num_threads * tasks_per_thread);
    tang::runtime::stop();
    std::cout << "工作窃取模拟测试通过!" << std::endl;
}

void test_cancellation_simulation() {
    std::cout << "测试取消模拟..." << std::endl;
    const int num_workers = 4;
    tang::channel<int> work_queue(20);
    tang::channel<bool> cancel_signal(1);
    std::atomic_int work_processed{0};
    std::atomic_int workers_active{0};
    
    tang::runtime::init(4);
    
    for (int i = 0; i < 20; ++i) {
        work_queue << i;
    }
    
    for (int w = 0; w < num_workers; ++w) {
        tang::go([&work_queue, &cancel_signal, &work_processed, &workers_active]() {
            workers_active++;
            while (true) {
                bool cancelled = false;
                if (cancel_signal.try_recv(cancelled)) {
                    break;
                }
                
                int value;
                if (work_queue.try_recv(value)) {
                    work_processed++;
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
            workers_active--;
        });
    }
    
    tang::runtime::sleep_ms(50);
    
    cancel_signal << true;
    
    tang::runtime::run();
    
    ASSERT(work_processed.load() > 0);
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
