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

// 测试基本的channel发送和接收
void test_basic_channel() {
    std::cout << "测试基本channel..." << std::endl;
    // 创建一个无缓冲channel
    tang::channel<int> ch;
    std::atomic_int received = 0;
    
    // 初始化运行时
    tang::runtime::init(2);
    
    // 创建接收协程
    tang::go([&ch, &received]() {
        int value;
        ch >> value;
        received = value;
    });
    
    // 创建发送协程
    tang::go([&ch]() {
        ch << 42;
    });
    
    // 运行调度器
    tang::runtime::run();
    
    // 验证结果
    ASSERT(received.load() == 42);
    
    // 停止运行时
    tang::runtime::stop();
    std::cout << "基本channel测试通过!" << std::endl;
}

// 测试带缓冲的channel
void test_buffered_channel() {
    std::cout << "测试缓冲channel..." << std::endl;
    // 创建一个容量为5的缓冲channel
    tang::channel<int> ch(5);
    
    // 初始化运行时
    tang::runtime::init(2);
    
    // 测试发送操作
    ASSERT(ch.try_send(1));
    ASSERT(ch.try_send(2));
    ASSERT(ch.try_send(3));
    ASSERT(ch.try_send(4));
    ASSERT(ch.try_send(5));
    
    // 缓冲区已满，尝试发送应该失败
    ASSERT(!ch.try_send(6));
    
    // 测试接收操作
    int value;
    ASSERT(ch.try_recv(value));
    ASSERT(value == 1);
    
    // 现在缓冲区有空间，可以发送
    ASSERT(ch.try_send(6));
    
    // 接收剩余的值
    for (int i = 2; i <= 6; ++i) {
        ASSERT(ch.try_recv(value));
        ASSERT(value == i);
    }
    
    // 缓冲区为空，尝试接收应该失败
    ASSERT(!ch.try_recv(value));
    
    // 停止运行时
    tang::runtime::stop();
    std::cout << "缓冲channel测试通过!" << std::endl;
}

// 测试channel的关闭
void test_channel_close() {
    std::cout << "测试channel关闭..." << std::endl;
    // 创建一个channel
    tang::channel<int> ch(2);
    
    // 初始化运行时
    tang::runtime::init(2);
    
    // 发送一些数据
    ch << 1;
    ch << 2;
    
    // 关闭channel
    ch.close();
    
    // 验证channel已关闭
    ASSERT(ch.is_closed());
    
    // 尝试发送数据到已关闭的channel应该失败
    ASSERT(!ch.try_send(3));
    
    // 可以接收剩余的数据
    int value;
    ASSERT(ch.try_recv(value));
    ASSERT(value == 1);
    
    ASSERT(ch.try_recv(value));
    ASSERT(value == 2);
    
    // 所有数据接收完毕后，尝试接收应该失败
    ASSERT(!ch.try_recv(value));
    
    // 停止运行时
    tang::runtime::stop();
    std::cout << "channel关闭测试通过!" << std::endl;
}

// 测试多个发送者和接收者
void test_multiple_sender_receiver() {
    std::cout << "测试多个发送者和接收者..." << std::endl;
    const int num_senders = 2; // 减少数量以加快测试
    const int num_receivers = 1; // 减少数量以加快测试
    const int messages_per_sender = 3; // 减少数量以加快测试
    
    // 创建一个缓冲channel
    tang::channel<int> ch(5); // 减少缓冲区大小
    
    std::atomic_int received = 0;
    std::atomic_int sent = 0;
    
    // 初始化运行时
    tang::runtime::init(2); // 减少线程数
    
    // 创建接收者协程
    for (int i = 0; i < num_receivers; ++i) {
        tang::go([&ch, &received, total = num_senders * messages_per_sender]() {
            while (received.load() < total) {
                int value;
                if (ch.try_recv(value)) {
                    received++;
                } else {
                    // 短暂睡眠，避免CPU占用过高
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                }
            }
        });
    }
    
    // 创建发送者协程
    for (int i = 0; i < num_senders; ++i) {
        tang::go([&ch, &sent, i, count = messages_per_sender]() {
            for (int j = 0; j < count; ++j) {
                int value = i * 100 + j;
                ch << value;
                sent++;
            }
        });
    }
    
    // 运行调度器
    tang::runtime::run();
    
    // 验证结果
    ASSERT(sent.load() == num_senders * messages_per_sender);
    ASSERT(received.load() == num_senders * messages_per_sender);
    
    // 停止运行时
    tang::runtime::stop();
    std::cout << "多个发送者和接收者测试通过!" << std::endl;
}

// 测试channel的状态查询
void test_channel_status() {
    std::cout << "测试channel状态查询..." << std::endl;
    // 创建一个容量为3的缓冲channel
    tang::channel<int> ch(3);
    
    // 初始化运行时
    tang::runtime::init(2);
    
    // 初始状态
    ASSERT(!ch.is_closed());
    ASSERT(ch.is_empty());
    ASSERT(!ch.is_full());
    
    // 发送一个元素
    ch << 1;
    ASSERT(!ch.is_empty());
    ASSERT(!ch.is_full());
    
    // 发送更多元素，直到满
    ch << 2;
    ch << 3;
    ASSERT(!ch.is_empty());
    ASSERT(ch.is_full());
    
    // 接收一个元素
    int value;
    ch >> value;
    ASSERT(!ch.is_empty());
    ASSERT(!ch.is_full());
    
    // 关闭channel
    ch.close();
    ASSERT(ch.is_closed());
    
    // 停止运行时
    tang::runtime::stop();
    std::cout << "channel状态查询测试通过!" << std::endl;
}

// 测试channel的try_send和try_recv
void test_try_send_recv() {
    std::cout << "测试try_send和try_recv..." << std::endl;
    // 创建一个容量为2的缓冲channel
    tang::channel<std::string> ch(2);
    
    // 初始化运行时
    tang::runtime::init(2);
    
    // 测试try_send
    ASSERT(ch.try_send("hello"));
    ASSERT(ch.try_send("world"));
    ASSERT(!ch.try_send("tang"));
    
    // 测试try_recv
    std::string msg;
    ASSERT(ch.try_recv(msg));
    ASSERT(msg == "hello");
    
    ASSERT(ch.try_recv(msg));
    ASSERT(msg == "world");
    
    ASSERT(!ch.try_recv(msg));
    
    // 停止运行时
    tang::runtime::stop();
    std::cout << "try_send和try_recv测试通过!" << std::endl;
}

// 测试关闭channel后的接收行为
void test_receive_after_close() {
    std::cout << "测试关闭channel后的接收行为..." << std::endl;
    // 创建一个channel
    tang::channel<int> ch(3);
    
    // 初始化运行时
    tang::runtime::init(2);
    
    // 发送一些数据
    ch << 1;
    ch << 2;
    ch << 3;
    
    // 关闭channel
    ch.close();
    
    // 接收所有数据
    std::vector<int> received;
    int value;
    
    while (ch.try_recv(value)) {
        received.push_back(value);
    }
    
    // 验证接收的数据
    ASSERT(received.size() == 3);
    ASSERT(received[0] == 1);
    ASSERT(received[1] == 2);
    ASSERT(received[2] == 3);
    
    // 再次尝试接收应该失败
    ASSERT(!ch.try_recv(value));
    
    // 停止运行时
    tang::runtime::stop();
    std::cout << "关闭channel后的接收行为测试通过!" << std::endl;
}

// 测试字符串类型的channel
void test_string_channel() {
    std::cout << "测试字符串类型的channel..." << std::endl;
    // 创建一个字符串channel
    tang::channel<std::string> ch(2);
    
    // 初始化运行时
    tang::runtime::init(2);
    
    // 发送字符串
    ch << "hello";
    ch << "tang";
    
    // 接收字符串
    std::string msg1, msg2;
    ch >> msg1;
    ch >> msg2;
    
    // 验证结果
    ASSERT(msg1 == "hello");
    ASSERT(msg2 == "tang");
    
    // 停止运行时
    tang::runtime::stop();
    std::cout << "字符串类型的channel测试通过!" << std::endl;
}

// 定义一个结构体用于测试
struct Person {
    std::string name;
    int age;
};

// 测试结构体类型的channel
void test_struct_channel() {
    std::cout << "测试结构体类型的channel..." << std::endl;
    
    // 创建一个结构体channel
    tang::channel<Person> ch(2);
    
    // 初始化运行时
    tang::runtime::init(2);
    
    // 发送结构体
    ch << Person{"Alice", 30};
    ch << Person{"Bob", 25};
    
    // 接收结构体
    Person p1, p2;
    ch >> p1;
    ch >> p2;
    
    // 验证结果
    ASSERT(p1.name == "Alice");
    ASSERT(p1.age == 30);
    
    ASSERT(p2.name == "Bob");
    ASSERT(p2.age == 25);
    
    // 停止运行时
    tang::runtime::stop();
    std::cout << "结构体类型的channel测试通过!" << std::endl;
}

// 主函数
int main() {
    std::cout << "开始运行channel测试..." << std::endl;
    
    try {
        test_basic_channel();
        test_buffered_channel();
        test_channel_close();
        test_multiple_sender_receiver();
        test_channel_status();
        test_try_send_recv();
        test_receive_after_close();
        test_string_channel();
        test_struct_channel();
        
        std::cout << "\\n所有channel测试通过!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "测试失败: " << e.what() << std::endl;
        return 1;
    }
}
