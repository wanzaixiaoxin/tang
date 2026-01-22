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

// 测试基本的channel操作
void test_basic_channel() {
    std::cout << "测试基本channel操作..." << std::endl;
    // 创建一个channel
    tang::channel<int> ch;
    
    int received = 0;
    
    // 初始化运行时
    tang::runtime::init(2);
    
    // 创建接收协程
    tang::go([&ch, &received]() -> tang::task<void> {
        ch >> received;
        co_return;
    });
    
    // 发送数据
    ch << 42;
    
    // 运行调度器
    tang::runtime::run();
    
    // 验证结果
    ASSERT(received == 42);
    
    std::cout << "基本channel操作测试通过!" << std::endl;
}

// 主函数
int main() {
    std::cout << "开始运行简单测试..." << std::endl;
    
    try {
        test_basic_channel();
        std::cout << "\n所有简单测试通过!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "测试失败: " << e.what() << std::endl;
        return 1;
    }
}