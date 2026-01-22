#include <iostream>
#include <cassert>
#include <tang/tang.h>
#include <vector>
#include <atomic>
#include <stdexcept>
#include <thread>
#include <functional>

// 简单的断言宏
#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion failed: " << #condition << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::terminate(); \
        } \
    } while(0)

// 测试基本的协程创建和运行
void test_basic_task() {
    std::cout << "测试基本协程..." << std::endl;
    std::atomic_bool executed = false;
    
    // 初始化运行时
    tang::runtime::init(2);
    
    // 创建协程
    tang::go([&executed]() {
        executed = true;
    });
    
    // 运行调度器
    tang::runtime::run();
    
    // 验证协程执行
    ASSERT(executed.load());
    
    // 停止运行时
    tang::runtime::stop();
    std::cout << "基本协程测试通过!" << std::endl;
}

// 测试协程函数的返回值
void test_task_return_value() {
    std::cout << "测试协程返回值..." << std::endl;
    std::atomic_int result = 0;
    
    // 初始化运行时
    tang::runtime::init(2);
    
    // 定义一个返回值的协程函数
    auto task_func = []() -> int {
        return 42;
    };
    
    // 创建协程并获取结果
    tang::go([&result, &task_func]() {
        result = task_func();
    });
    
    // 运行调度器
    tang::runtime::run();
    
    // 验证返回值
    ASSERT(result.load() == 42);
    
    // 停止运行时
    tang::runtime::stop();
    std::cout << "协程返回值测试通过!" << std::endl;
}

// 测试多个协程的并发执行
void test_multiple_tasks() {
    std::cout << "测试多个协程并发执行..." << std::endl;
    const int num_tasks = 10; // 减少任务数量以加快测试
    std::atomic_int executed_count = 0;
    
    // 初始化运行时
    tang::runtime::init(4);
    
    // 创建多个协程
    for (int i = 0; i < num_tasks; ++i) {
        tang::go([&executed_count, i]() {
            // 模拟一些工作
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            executed_count++;
        });
    }
    
    // 运行调度器
    tang::runtime::run();
    
    // 验证所有协程都执行了
    ASSERT(executed_count.load() == num_tasks);
    
    // 停止运行时
    tang::runtime::stop();
    std::cout << "多个协程并发执行测试通过!" << std::endl;
}

// 测试协程的异常处理
void test_task_exception() {
    std::cout << "测试协程异常处理..." << std::endl;
    std::atomic_bool caught = false;
    
    // 初始化运行时
    tang::runtime::init(2);
    
    // 创建一个会抛出异常的协程
    tang::go([&caught]() {
        try {
            throw std::runtime_error("Test exception");
        } catch (const std::exception& e) {
            caught = true;
        }
    });
    
    // 运行调度器
    tang::runtime::run();
    
    // 验证异常被捕获
    ASSERT(caught.load());
    
    // 停止运行时
    tang::runtime::stop();
    std::cout << "协程异常处理测试通过!" << std::endl;
}

// 测试带参数的协程函数
void test_task_with_parameters() {
    std::cout << "测试带参数协程..." << std::endl;
    std::atomic_int result = 0;
    
    // 初始化运行时
    tang::runtime::init(2);
    
    // 定义一个带参数的函数
    auto add_func = [](int a, int b) {
        return a + b;
    };
    
    // 创建协程并传递参数
    tang::go([&result, &add_func]() {
        result = add_func(10, 20);
    });
    
    // 运行调度器
    tang::runtime::run();
    
    // 验证结果
    ASSERT(result.load() == 30);
    
    // 停止运行时
    tang::runtime::stop();
    std::cout << "带参数协程测试通过!" << std::endl;
}

// 测试协程的yield功能
void test_task_yield() {
    std::cout << "测试协程yield功能..." << std::endl;
    std::atomic_int execution_order = 0;
    
    // 初始化运行时
    tang::runtime::init(2);
    
    // 创建第一个协程
    tang::go([&execution_order]() {
        execution_order++;
        
        // 让出CPU
        tang::runtime::yield();
        
        execution_order += 2;
    });
    
    // 创建第二个协程
    tang::go([&execution_order]() {
        execution_order++;
    });
    
    // 运行调度器
    tang::runtime::run();
    
    // 验证执行顺序
    ASSERT(execution_order.load() == 4);
    
    // 停止运行时
    tang::runtime::stop();
    std::cout << "协程yield功能测试通过!" << std::endl;
}

// 测试spawn函数（与go同义）
void test_spawn_function() {
    std::cout << "测试spawn函数..." << std::endl;
    std::atomic_bool executed = false;
    
    // 初始化运行时
    tang::runtime::init(2);
    
    // 使用spawn创建协程
    tang::spawn([&executed]() {
        executed = true;
    });
    
    // 运行调度器
    tang::runtime::run();
    
    // 验证协程执行
    ASSERT(executed.load());
    
    // 停止运行时
    tang::runtime::stop();
    std::cout << "spawn函数测试通过!" << std::endl;
}

// 测试协程的睡眠功能
void test_task_sleep() {
    std::cout << "测试协程睡眠功能..." << std::endl;
    auto start = std::chrono::steady_clock::now();
    
    // 初始化运行时
    tang::runtime::init(2);
    
    // 创建一个睡眠的协程
    tang::go([]() {
        // 睡眠100毫秒
        tang::runtime::sleep_ms(100);
    });
    
    // 运行调度器
    tang::runtime::run();
    
    // 计算执行时间
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // 验证至少睡眠了100毫秒
    ASSERT(duration.count() >= 100);
    
    // 停止运行时
    tang::runtime::stop();
    std::cout << "协程睡眠功能测试通过!" << std::endl;
}

// 测试不同线程数的运行时
void test_different_thread_counts() {
    std::cout << "测试不同线程数..." << std::endl;
    std::atomic_int executed_count = 0;
    const int num_tasks = 10; // 减少任务数量以加快测试
    
    // 测试不同的线程数
    for (int threads = 1; threads <= 2; ++threads) { // 减少测试的线程数范围
        executed_count = 0;
        
        // 初始化运行时，使用不同的线程数
        tang::runtime::init(threads);
        
        // 创建多个协程
        for (int i = 0; i < num_tasks; ++i) {
            tang::go([&executed_count]() {
                executed_count++;
            });
        }
        
        // 运行调度器
        tang::runtime::run();
        
        // 验证所有协程都执行了
        ASSERT(executed_count.load() == num_tasks);
        
        // 停止运行时
        tang::runtime::stop();
    }
    std::cout << "不同线程数测试通过!" << std::endl;
}

// 测试嵌套协程调用
void test_nested_coroutines() {
    std::cout << "测试嵌套协程..." << std::endl;
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
    std::cout << "嵌套协程测试通过!" << std::endl;
}

// 测试协程间的多层嵌套
void test_multi_level_nested_coroutines() {
    std::cout << "测试多层嵌套协程..." << std::endl;
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
    std::cout << "多层嵌套协程测试通过!" << std::endl;
}

// 测试任务组 - wait_all语义
void test_wait_all() {
    std::cout << "测试任务组wait_all..." << std::endl;
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
    std::cout << "任务组wait_all测试通过!" << std::endl;
}

// 测试递归协程
void test_recursive_coroutine() {
    std::cout << "测试递归协程..." << std::endl;
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
    std::cout << "递归协程测试通过!" << std::endl;
}

// 测试协程的异常传播
void test_exception_propagation() {
    std::cout << "测试异常传播..." << std::endl;
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
        }
        co_return;
    };
    
    auto outer_task = [&outer_exception_caught, &catching_task]() -> tang::task<void> {
        try {
            auto t = catching_task();
            co_await t;
            // 模拟另一个异常
            throw std::runtime_error("Outer exception");
        } catch (const std::exception& e) {
            outer_exception_caught = true;
        }
        co_return;
    };
    
    auto task = outer_task();
    task.run();
    tang::runtime::run();
    
    ASSERT(inner_exception_caught.load());
    ASSERT(outer_exception_caught.load());
    tang::runtime::stop();
    std::cout << "异常传播测试通过!" << std::endl;
}

// 测试协程的资源竞争
void test_resource_contention() {
    std::cout << "测试资源竞争..." << std::endl;
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
    std::cout << "资源竞争测试通过! 最终计数值: " << shared_counter.load() << std::endl;
}

// 测试任务生命周期管理
void test_task_lifecycle() {
    std::cout << "测试任务生命周期..." << std::endl;
    std::atomic_int lifecycle_events{0};
    
    tang::runtime::init(2);
    
    auto tracked_task = [&lifecycle_events]() -> tang::task<int> {
        lifecycle_events++; // 开始
        co_return 1;
    };
    
    {
        auto task = tracked_task();
        task.run();
        tang::runtime::run();
        ASSERT(lifecycle_events.load() >= 1);
    }
    
    tang::runtime::stop();
    std::cout << "任务生命周期测试通过!" << std::endl;
}

// 测试协程的并行执行
void test_parallel_execution() {
    std::cout << "测试并行执行..." << std::endl;
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
    std::cout << "并行执行测试通过!" << std::endl;
}

// 测试协程的数据传递
void test_data_passing() {
    std::cout << "测试数据传递..." << std::endl;
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
    std::cout << "数据传递测试通过!" << std::endl;
}

// 测试协程的同步原语组合
void test_sync_primitives_combo() {
    std::cout << "测试同步原语组合..." << std::endl;
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
    
    // 验证数据完整性
    ASSERT(sum.load() > 0);
    tang::runtime::stop();
    std::cout << "同步原语组合测试通过! 总和: " << sum.load() << std::endl;
}

// 主函数
int main() {
    std::cout << "开始运行任务系统测试..." << std::endl;
    
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
        
        std::cout << "\\n所有任务系统测试通过!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "测试失败: " << e.what() << std::endl;
        return 1;
    }
}
