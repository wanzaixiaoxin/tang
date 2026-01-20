#include <tang/tang.h>
#include <iostream>
#include <thread>

// 向通道发送数据的协程函数
void send_data(tang::channel<int>& ch, int id, int delay_ms, int count) {
    for (int i = 0; i < count; ++i) {
        int value = id * 100 + i;
        
        // 使用发送操作符
        ch << value;
        
        std::cout << "Sender " << id << " sent: " << value << " Thread ID: " << std::this_thread::get_id() << std::endl;
        
        // 模拟工作延迟
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    }
    
    // 关闭通道
    ch.close();
}

// 使用select的协程函数
void select_example() {
    std::cout << "\n=== Select Example ===" << std::endl;
    
    // 创建三个通道
    tang::channel<int> ch1(5);
    tang::channel<int> ch2(5);
    tang::channel<int> ch3(5);
    
    // 启动三个发送者协程，不同的发送频率
    tang::go(send_data, ch1, 1, 100, 5);  // 每100ms发送一次，共5次
    tang::go(send_data, ch2, 2, 200, 5);  // 每200ms发送一次，共5次
    tang::go(send_data, ch3, 3, 300, 5);  // 每300ms发送一次，共5次
    
    // 接收计数器
    int received = 0;
    const int total = 15;
    
    // 使用select接收数据
    while (received < total) {
        int value;
        
        // 使用select等待多个channel
        tang::select(
            // 接收case 1
            tang::case_recv(ch1, value, [&]() {
                std::cout << "Select received from ch1: " << value << " Thread ID: " << std::this_thread::get_id() << std::endl;
                received++;
            }),
            
            // 接收case 2
            tang::case_recv(ch2, value, [&]() {
                std::cout << "Select received from ch2: " << value << " Thread ID: " << std::this_thread::get_id() << std::endl;
                received++;
            }),
            
            // 接收case 3
            tang::case_recv(ch3, value, [&]() {
                std::cout << "Select received from ch3: " << value << " Thread ID: " << std::this_thread::get_id() << std::endl;
                received++;
            })
        );
    }
    
    std::cout << "Select example completed!" << std::endl;
}

// 使用默认case的select示例
void select_with_default_example() {
    std::cout << "\n=== Select with Default Case Example ===" << std::endl;
    
    // 创建一个通道
    tang::channel<int> ch;
    
    // 启动一个发送者协程，延迟发送
    tang::go([&ch]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        ch << 42;
        std::cout << "Sender sent: 42" << std::endl;
        ch.close();
    });
    
    // 使用select接收数据，带有默认case
    int received_count = 0;
    const int max_attempts = 10;
    
    for (int i = 0; i < max_attempts; ++i) {
        int value;
        bool has_value = false;
        
        tang::select(
            // 接收case
            tang::case_recv(ch, value, [&]() {
                std::cout << "Select received: " << value << std::endl;
                received_count++;
                has_value = true;
            }),
            
            // 默认case
            tang::default_case([&]() {
                std::cout << "Select default case executed" << std::endl;
            })
        );
        
        if (has_value) {
            break;
        }
        
        // 短暂延迟
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "Select with default example completed! Received " << received_count << " values" << std::endl;
}

int main() {
    std::cout << "Main thread ID: " << std::this_thread::get_id() << std::endl;
    
    // 初始化运行时，使用4个工作线程
    tang::runtime::init(4);
    
    // 运行select示例
    select_example();
    
    // 运行带有默认case的select示例
    select_with_default_example();
    
    // 运行调度器，阻塞直到所有协程完成
    tang::runtime::run();
    
    std::cout << "\nAll select examples completed!" << std::endl;
    
    return 0;
}
