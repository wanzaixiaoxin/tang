#include <iostream>
#include <cassert>
#include <tang/tang.h>
#include <vector>
#include <atomic>
#include <stdexcept>
#include <thread>
#include <chrono>

#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion failed: " << #condition << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::terminate(); \
        } \
    } while(0)

void test_runtime_init_single_thread() {
    std::cout << "测试单线程运行时初始化..." << std::endl;
    
    tang::runtime::init(1);
    
    std::atomic_int counter{0};
    for (int i = 0; i < 10; ++i) {
        tang::go([&counter]() {
            counter++;
        });
    }
    
    tang::runtime::run();
    ASSERT(counter.load() == 10);
    
    tang::runtime::stop();
    std::cout << "单线程运行时初始化测试通过!" << std::endl;
}

void test_runtime_init_multi_thread() {
    std::cout << "测试多线程运行时初始化..." << std::endl;
    
    tang::runtime::init(4);
    
    std::atomic_int counter{0};
    for (int i = 0; i < 20; ++i) {
        tang::go([&counter]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            counter++;
        });
    }
    
    tang::runtime::run();
    ASSERT(counter.load() == 20);
    
    tang::runtime::stop();
    std::cout << "多线程运行时初始化测试通过!" << std::endl;
}

void test_runtime_yield() {
    std::cout << "测试运行时yield..." << std::endl;
    
    tang::runtime::init(2);
    
    std::atomic_int order{0};
    int task1_order = 0, task2_order = 0;
    
    tang::go([&order, &task1_order]() {
            task1_order = ++order;
            tang::runtime::yield();
            task1_order = ++order;
        });
        
        tang::go([&order, &task2_order]() {
            task2_order = ++order;
        });
    
    tang::runtime::run();
    
    ASSERT(task1_order == 1 || task2_order == 1);
    ASSERT(task1_order == 3 && task2_order == 2);
    
    tang::runtime::stop();
    std::cout << "运行时yield测试通过!" << std::endl;
}

void test_runtime_sleep_ms() {
    std::cout << "测试运行时sleep_ms..." << std::endl;
    
    tang::runtime::init(2);
    
    auto start = std::chrono::steady_clock::now();
    
    tang::go([]() {
        tang::runtime::sleep_ms(50);
    });
    
    tang::runtime::run();
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    ASSERT(duration.count() >= 45);
    
    tang::runtime::stop();
    std::cout << "运行时sleep_ms测试通过!" << std::endl;
}

void test_runtime_multiple_init_stop_cycles() {
    std::cout << "测试多次init/stop周期..." << std::endl;
    
    for (int cycle = 0; cycle < 3; ++cycle) {
        tang::runtime::init(2);
        
        std::atomic_int counter{0};
        for (int i = 0; i < 5; ++i) {
            tang::go([&counter]() {
                counter++;
            });
        }
        
        tang::runtime::run();
        ASSERT(counter.load() == 5);
        
        tang::runtime::stop();
    }
    
    std::cout << "多次init/stop周期测试通过!" << std::endl;
}

void test_runtime_concurrent_tasks() {
    std::cout << "测试运行时并发任务..." << std::endl;
    
    tang::runtime::init(4);
    
    const int num_tasks = 100;
    std::atomic_int completed{0};
    std::atomic_int sum{0};
    
    for (int i = 0; i < num_tasks; ++i) {
        tang::go([&completed, &sum, i]() {
            sum += i;
            completed++;
        });
    }
    
    tang::runtime::run();
    
    ASSERT(completed.load() == num_tasks);
    
    int expected_sum = 0;
    for (int i = 0; i < num_tasks; ++i) {
        expected_sum += i;
    }
    ASSERT(sum.load() == expected_sum);
    
    tang::runtime::stop();
    std::cout << "运行时并发任务测试通过!" << std::endl;
}

void test_runtime_work_distribution() {
    std::cout << "测试运行时工作分配..." << std::endl;
    
    tang::runtime::init(4);
    
    const int tasks_per_thread = 10;
    std::vector<std::atomic_int> thread_counts(4);
    std::atomic_int total{0};
    
    for (int t = 0; t < 4; ++t) {
        tang::go([&thread_counts, &total, t, tasks_per_thread]() {
            for (int i = 0; i < tasks_per_thread; ++i) {
                thread_counts[t]++;
                total++;
            }
        });
    }
    
    tang::runtime::run();
    
    ASSERT(total.load() == 4 * tasks_per_thread);
    
    tang::runtime::stop();
    std::cout << "运行时工作分配测试通过!" << std::endl;
}

void test_runtime_cpu_intensive_tasks() {
    std::cout << "测试运行时CPU密集型任务..." << std::endl;
    
    tang::runtime::init(4);
    
    const int num_tasks = 8;
    std::atomic_int completed{0};
    
    for (int i = 0; i < num_tasks; ++i) {
        tang::go([&completed, i]() {
            volatile int result = 0;
            for (int j = 0; j < 100000; ++j) {
                result += j * i;
            }
            completed++;
        });
    }
    
    tang::runtime::run();
    ASSERT(completed.load() == num_tasks);
    
    tang::runtime::stop();
    std::cout << "运行时CPU密集型任务测试通过!" << std::endl;
}

void test_runtime_io_wait_tasks() {
    std::cout << "测试运行时IO等待任务..." << std::endl;
    
    tang::runtime::init(4);
    
    const int num_tasks = 4;
    std::atomic_int completed{0};
    
    for (int i = 0; i < num_tasks; ++i) {
        tang::go([&completed, i]() {
            for (int j = 0; j < 3; ++j) {
                tang::runtime::sleep_ms(10);
            }
            completed++;
        });
    }
    
    tang::runtime::run();
    ASSERT(completed.load() == num_tasks);
    
    tang::runtime::stop();
    std::cout << "运行时IO等待任务测试通过!" << std::endl;
}

void test_runtime_mixed_tasks() {
    std::cout << "测试运行时混合任务..." << std::endl;
    
    tang::runtime::init(4);
    
    const int cpu_tasks = 4;
    const int io_tasks = 4;
    std::atomic_int cpu_completed{0};
    std::atomic_int io_completed{0};
    
    for (int i = 0; i < cpu_tasks; ++i) {
        tang::go([&cpu_completed, i]() {
            volatile int result = 0;
            for (int j = 0; j < 50000; ++j) {
                result += j * i;
            }
            cpu_completed++;
        });
    }
    
    for (int i = 0; i < io_tasks; ++i) {
        tang::go([&io_completed, i]() {
            for (int j = 0; j < 3; ++j) {
                tang::runtime::sleep_ms(5);
            }
            io_completed++;
        });
    }
    
    tang::runtime::run();
    
    ASSERT(cpu_completed.load() == cpu_tasks);
    ASSERT(io_completed.load() == io_tasks);
    
    tang::runtime::stop();
    std::cout << "运行时混合任务测试通过!" << std::endl;
}

void test_runtime_task_affinity() {
    std::cout << "测试运行时任务亲和性..." << std::endl;
    
    tang::runtime::init(2);
    
    std::atomic_int task1_runs{0};
    std::atomic_int task2_runs{0};
    
    tang::go([&task1_runs]() {
        for (int i = 0; i < 5; ++i) {
            task1_runs++;
            tang::runtime::yield();
        }
    });
    
    tang::go([&task2_runs]() {
        for (int i = 0; i < 5; ++i) {
            task2_runs++;
            tang::runtime::yield();
        }
    });
    
    tang::runtime::run();
    
    ASSERT(task1_runs.load() == 5);
    ASSERT(task2_runs.load() == 5);
    
    tang::runtime::stop();
    std::cout << "运行时任务亲和性测试通过!" << std::endl;
}

int main() {
    std::cout << "开始运行运行时测试..." << std::endl;
    
    try {
        test_runtime_init_single_thread();
        test_runtime_init_multi_thread();
        test_runtime_yield();
        test_runtime_sleep_ms();
        test_runtime_multiple_init_stop_cycles();
        test_runtime_concurrent_tasks();
        test_runtime_work_distribution();
        test_runtime_cpu_intensive_tasks();
        test_runtime_io_wait_tasks();
        test_runtime_mixed_tasks();
        test_runtime_task_affinity();
        
        std::cout << "\n所有运行时测试通过!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "测试失败: " << e.what() << std::endl;
        return 1;
    }
}
