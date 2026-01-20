#include <tang/tang.h>
#include <iostream>
#include <thread>
#include <vector>

// 发送者协程函数
void sender(tang::channel<int>& ch, int id, int count) {
    for (int i = 0; i < count; ++i) {
        int value = id * 100 + i;
        
        // 使用发送操作符
        ch << value;
        
        std::cout << "Sender " << id << " sent: " << value << " Thread ID: " << std::this_thread::get_id() << std::endl;
        
        // 模拟工作
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// 接收者协程函数
void receiver(tang::channel<int>& ch, int id, int count) {
    for (int i = 0; i < count; ++i) {
        int value;
        
        // 使用接收操作符
        ch >> value;
        
        std::cout << "Receiver " << id << " received: " << value << " Thread ID: " << std::this_thread::get_id() << std::endl;
        
        // 模拟工作
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
}

// 带缓冲区的channel示例
void buffered_channel_example() {
    std::cout << "\n=== Buffered Channel Example ===" << std::endl;
    
    // 创建一个容量为5的缓冲通道
    tang::channel<int> ch(5);
    
    // 启动2个发送者协程，每个发送10个数据
    tang::go(sender, &ch, 1, 10);
    tang::go(sender, &ch, 2, 10);
    
    // 启动2个接收者协程，每个接收10个数据
    tang::go(receiver, &ch, 1, 10);
    tang::go(receiver, &ch, 2, 10);
}

// 无缓冲区的channel示例
void unbuffered_channel_example() {
    std::cout << "\n=== Unbuffered Channel Example ===" << std::endl;
    
    // 创建一个无缓冲通道
    tang::channel<std::string> ch;
    
    // 启动发送者协程
    tang::go([&ch]() {
        std::vector<std::string> messages = {"Hello", "from", "unbuffered", "channel"};
        
        for (const auto& msg : messages) {
            ch << msg;
            std::cout << "Sent: " << msg << std::endl;
        }
        
        // 关闭通道
        ch.close();
    });
    
    // 启动接收者协程
    tang::go([&ch]() {
        std::string msg;
        
        // 从通道接收数据，直到通道关闭
        while (ch >> msg) {
            std::cout << "Received: " << msg << std::endl;
        }
        
        std::cout << "Channel closed" << std::endl;
    });
}

int main() {
    std::cout << "Main thread ID: " << std::this_thread::get_id() << std::endl;
    
    // 初始化运行时，使用4个工作线程
    tang::runtime::init(4);
    
    // 运行带缓冲区的channel示例
    buffered_channel_example();
    
    // 运行无缓冲区的channel示例
    unbuffered_channel_example();
    
    // 运行调度器，阻塞直到所有协程完成
    tang::runtime::run();
    
    std::cout << "\nAll examples completed!" << std::endl;
    
    return 0;
}
