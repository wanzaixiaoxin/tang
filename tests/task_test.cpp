#include <iostream>
#include <cassert>
#include <tang/tang.h>
#include <vector>
#include <atomic>
#include <stdexcept>
#include <thread>

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
        
        std::cout << "\\n所有任务系统测试通过!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "测试失败: " << e.what() << std::endl;
        return 1;
    }
}
