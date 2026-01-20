#include <gtest/gtest.h>
#include <tang/tang.h>
#include <vector>
#include <atomic>
#include <stdexcept>
#include <thread>

// 测试基本的channel发送和接收
TEST(ChannelTest, BasicChannel) {
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
    EXPECT_EQ(received.load(), 42);
    
    // 停止运行时
    tang::runtime::stop();
}

// 测试带缓冲的channel
TEST(ChannelTest, BufferedChannel) {
    // 创建一个容量为5的缓冲channel
    tang::channel<int> ch(5);
    
    // 初始化运行时
    tang::runtime::init(2);
    
    // 测试发送操作
    EXPECT_TRUE(ch.try_send(1));
    EXPECT_TRUE(ch.try_send(2));
    EXPECT_TRUE(ch.try_send(3));
    EXPECT_TRUE(ch.try_send(4));
    EXPECT_TRUE(ch.try_send(5));
    
    // 缓冲区已满，尝试发送应该失败
    EXPECT_FALSE(ch.try_send(6));
    
    // 测试接收操作
    int value;
    EXPECT_TRUE(ch.try_recv(value));
    EXPECT_EQ(value, 1);
    
    // 现在缓冲区有空间，可以发送
    EXPECT_TRUE(ch.try_send(6));
    
    // 接收剩余的值
    for (int i = 2; i <= 6; ++i) {
        EXPECT_TRUE(ch.try_recv(value));
        EXPECT_EQ(value, i);
    }
    
    // 缓冲区为空，尝试接收应该失败
    EXPECT_FALSE(ch.try_recv(value));
    
    // 停止运行时
    tang::runtime::stop();
}

// 测试channel的关闭
TEST(ChannelTest, ChannelClose) {
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
    EXPECT_TRUE(ch.is_closed());
    
    // 尝试发送数据到已关闭的channel应该失败
    EXPECT_FALSE(ch.try_send(3));
    
    // 可以接收剩余的数据
    int value;
    EXPECT_TRUE(ch.try_recv(value));
    EXPECT_EQ(value, 1);
    
    EXPECT_TRUE(ch.try_recv(value));
    EXPECT_EQ(value, 2);
    
    // 所有数据接收完毕后，尝试接收应该失败
    EXPECT_FALSE(ch.try_recv(value));
    
    // 停止运行时
    tang::runtime::stop();
}

// 测试多个发送者和接收者
TEST(ChannelTest, MultipleSenderReceiver) {
    const int num_senders = 3;
    const int num_receivers = 2;
    const int messages_per_sender = 5;
    
    // 创建一个缓冲channel
    tang::channel<int> ch(10);
    
    std::atomic_int received = 0;
    std::atomic_int sent = 0;
    
    // 初始化运行时
    tang::runtime::init(4);
    
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
    EXPECT_EQ(sent.load(), num_senders * messages_per_sender);
    EXPECT_EQ(received.load(), num_senders * messages_per_sender);
    
    // 停止运行时
    tang::runtime::stop();
}

// 测试channel的状态查询
TEST(ChannelTest, ChannelStatus) {
    // 创建一个容量为3的缓冲channel
    tang::channel<int> ch(3);
    
    // 初始化运行时
    tang::runtime::init(2);
    
    // 初始状态
    EXPECT_FALSE(ch.is_closed());
    EXPECT_TRUE(ch.is_empty());
    EXPECT_FALSE(ch.is_full());
    EXPECT_EQ(ch.capacity(), 3);
    EXPECT_EQ(ch.size(), 0);
    
    // 发送一个元素
    ch << 1;
    EXPECT_FALSE(ch.is_empty());
    EXPECT_FALSE(ch.is_full());
    EXPECT_EQ(ch.size(), 1);
    
    // 发送更多元素，直到满
    ch << 2;
    ch << 3;
    EXPECT_FALSE(ch.is_empty());
    EXPECT_TRUE(ch.is_full());
    EXPECT_EQ(ch.size(), 3);
    
    // 接收一个元素
    int value;
    ch >> value;
    EXPECT_FALSE(ch.is_empty());
    EXPECT_FALSE(ch.is_full());
    EXPECT_EQ(ch.size(), 2);
    
    // 关闭channel
    ch.close();
    EXPECT_TRUE(ch.is_closed());
    
    // 停止运行时
    tang::runtime::stop();
}

// 测试channel的try_send和try_recv
TEST(ChannelTest, TrySendRecv) {
    // 创建一个容量为2的缓冲channel
    tang::channel<std::string> ch(2);
    
    // 初始化运行时
    tang::runtime::init(2);
    
    // 测试try_send
    EXPECT_TRUE(ch.try_send("hello"));
    EXPECT_TRUE(ch.try_send("world"));
    EXPECT_FALSE(ch.try_send("tang"));
    
    // 测试try_recv
    std::string msg;
    EXPECT_TRUE(ch.try_recv(msg));
    EXPECT_EQ(msg, "hello");
    
    EXPECT_TRUE(ch.try_recv(msg));
    EXPECT_EQ(msg, "world");
    
    EXPECT_FALSE(ch.try_recv(msg));
    
    // 停止运行时
    tang::runtime::stop();
}

// 测试关闭channel后的接收行为
TEST(ChannelTest, ReceiveAfterClose) {
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
    EXPECT_EQ(received.size(), 3);
    EXPECT_EQ(received[0], 1);
    EXPECT_EQ(received[1], 2);
    EXPECT_EQ(received[2], 3);
    
    // 再次尝试接收应该失败
    EXPECT_FALSE(ch.try_recv(value));
    
    // 停止运行时
    tang::runtime::stop();
}

// 测试字符串类型的channel
TEST(ChannelTest, StringChannel) {
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
    EXPECT_EQ(msg1, "hello");
    EXPECT_EQ(msg2, "tang");
    
    // 停止运行时
    tang::runtime::stop();
}

// 测试结构体类型的channel
TEST(ChannelTest, StructChannel) {
    // 定义一个结构体
    struct Person {
        std::string name;
        int age;
    };
    
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
    EXPECT_EQ(p1.name, "Alice");
    EXPECT_EQ(p1.age, 30);
    
    EXPECT_EQ(p2.name, "Bob");
    EXPECT_EQ(p2.age, 25);
    
    // 停止运行时
    tang::runtime::stop();
}
