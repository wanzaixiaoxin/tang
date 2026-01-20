#include <gtest/gtest.h>
#include <tang/tang.h>
#include <vector>
#include <atomic>
#include <stdexcept>
#include <thread>

// 测试基本的协程创建和运行
TEST(TaskTest, BasicTask) {
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
    EXPECT_TRUE(executed.load());
    
    // 停止运行时
    tang::runtime::stop();
}

// 测试协程函数的返回值
TEST(TaskTest, TaskReturnValue) {
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
    EXPECT_EQ(result.load(), 42);
    
    // 停止运行时
    tang::runtime::stop();
}

// 测试多个协程的并发执行
TEST(TaskTest, MultipleTasks) {
    const int num_tasks = 100;
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
    EXPECT_EQ(executed_count.load(), num_tasks);
    
    // 停止运行时
    tang::runtime::stop();
}

// 测试协程的异常处理
TEST(TaskTest, TaskException) {
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
    EXPECT_TRUE(caught.load());
    
    // 停止运行时
    tang::runtime::stop();
}

// 测试带参数的协程函数
TEST(TaskTest, TaskWithParameters) {
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
    EXPECT_EQ(result.load(), 30);
    
    // 停止运行时
    tang::runtime::stop();
}

// 测试协程的yield功能
TEST(TaskTest, TaskYield) {
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
    EXPECT_EQ(execution_order.load(), 4);
    
    // 停止运行时
    tang::runtime::stop();
}

// 测试spawn函数（与go同义）
TEST(TaskTest, SpawnFunction) {
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
    EXPECT_TRUE(executed.load());
    
    // 停止运行时
    tang::runtime::stop();
}

// 测试协程的睡眠功能
TEST(TaskTest, TaskSleep) {
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
    EXPECT_GE(duration.count(), 100);
    
    // 停止运行时
    tang::runtime::stop();
}

// 测试不同线程数的运行时
TEST(TaskTest, DifferentThreadCounts) {
    std::atomic_int executed_count = 0;
    const int num_tasks = 50;
    
    // 测试不同的线程数
    for (int threads = 1; threads <= 4; ++threads) {
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
        EXPECT_EQ(executed_count.load(), num_tasks) << "Failed with " << threads << " threads";
        
        // 停止运行时
        tang::runtime::stop();
    }
}
