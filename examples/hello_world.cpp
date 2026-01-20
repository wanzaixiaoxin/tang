#include <tang/tang.h>
#include <iostream>
#include <thread>

// 简单的协程函数
void hello() {
    std::cout << "Hello from goroutine! Thread ID: " << std::this_thread::get_id() << std::endl;
}

// 带参数的协程函数
void hello_with_name(const std::string& name) {
    std::cout << "Hello, " << name << "! Thread ID: " << std::this_thread::get_id() << std::endl;
}

int main() {
    std::cout << "Main thread ID: " << std::this_thread::get_id() << std::endl;
    
    // 初始化运行时，默认使用CPU核心数作为工作线程数
    tang::runtime::init();
    
    // 启动10个协程
    for (int i = 0; i < 10; ++i) {
        // 使用go关键字启动协程
        tang::go(hello);
        
        // 使用spawn关键字启动协程（与go同义）
        tang::spawn(hello_with_name, "Tang");
        
        // 使用lambda表达式启动协程
        tang::go([i]() {
            std::cout << "Hello from lambda goroutine " << i << "! Thread ID: " << std::this_thread::get_id() << std::endl;
        });
    }
    
    // 运行调度器，阻塞直到所有协程完成
    tang::runtime::run();
    
    std::cout << "All goroutines completed!" << std::endl;
    
    return 0;
}
