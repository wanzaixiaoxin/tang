#include <tang/tang.h>
#include <iostream>
#include <fstream>
#include <mutex>
#include <vector>

// 简单的defer示例
void simple_defer() {
    std::cout << "Entering simple_defer()" << std::endl;
    
    // 使用defer注册一个退出时执行的函数
    defer {
        std::cout << "Exiting simple_defer() - defer executed" << std::endl;
    };
    
    std::cout << "Doing work in simple_defer()" << std::endl;
}

// 多个defer示例（逆序执行）
void multiple_defer() {
    std::cout << "\nEntering multiple_defer()" << std::endl;
    
    // 注册多个defer，将逆序执行
    defer {
        std::cout << "Exiting multiple_defer() - defer 1 executed" << std::endl;
    };
    
    defer {
        std::cout << "Exiting multiple_defer() - defer 2 executed" << std::endl;
    };
    
    defer {
        std::cout << "Exiting multiple_defer() - defer 3 executed" << std::endl;
    };
    
    std::cout << "Doing work in multiple_defer()" << std::endl;
}

// 资源清理示例 - 文件操作
void file_defer_example() {
    std::cout << "\nEntering file_defer_example()" << std::endl;
    
    // 打开文件
    std::ofstream file("test.txt");
    if (!file.is_open()) {
        std::cerr << "Failed to open file" << std::endl;
        return;
    }
    
    // 使用defer确保文件关闭
    defer {
        if (file.is_open()) {
            file.close();
            std::cout << "File closed by defer" << std::endl;
        }
    };
    
    // 写入数据
    file << "Hello, Tang!" << std::endl;
    file << "This is a test file." << std::endl;
    
    std::cout << "File operations completed" << std::endl;
    
    // 文件将在函数退出时由defer关闭
}

// 资源清理示例 - 互斥锁
void mutex_defer_example() {
    std::cout << "\nEntering mutex_defer_example()" << std::endl;
    
    static std::mutex mtx;
    static std::vector<int> shared_data;
    
    // 加锁
    mtx.lock();
    
    // 使用defer确保解锁
    defer {
        mtx.unlock();
        std::cout << "Mutex unlocked by defer" << std::endl;
    };
    
    // 访问共享数据
    shared_data.push_back(42);
    shared_data.push_back(100);
    shared_data.push_back(200);
    
    std::cout << "Shared data modified. Size: " << shared_data.size() << std::endl;
    
    // 锁将在函数退出时由defer释放
}

// 异常情况下的defer示例
void exception_defer_example() {
    std::cout << "\nEntering exception_defer_example()" << std::endl;
    
    try {
        // 注册defer
        defer {
            std::cout << "Exception defer executed" << std::endl;
        };
        
        std::cout << "Doing work before exception" << std::endl;
        
        // 抛出异常
        throw std::runtime_error("Test exception");
        
        std::cout << "This line will not be executed" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Caught exception: " << e.what() << std::endl;
    }
    
    std::cout << "Exiting exception_defer_example()" << std::endl;
}

int main() {
    std::cout << "Main thread ID: " << std::this_thread::get_id() << std::endl;
    
    // 初始化运行时，使用2个工作线程
    tang::runtime::init(2);
    
    // 运行各种defer示例
    simple_defer();
    multiple_defer();
    file_defer_example();
    mutex_defer_example();
    exception_defer_example();
    
    // 在协程中使用defer
    tang::go([]() {
        std::cout << "\nEntering goroutine" << std::endl;
        
        defer {
            std::cout << "Goroutine defer executed" << std::endl;
        };
        
        std::cout << "Doing work in goroutine" << std::endl;
        
        // 模拟工作
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::cout << "Goroutine work completed" << std::endl;
    });
    
    // 运行调度器，阻塞直到所有协程完成
    tang::runtime::run();
    
    std::cout << "\nAll defer examples completed!" << std::endl;
    
    return 0;
}
