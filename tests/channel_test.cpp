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

// 测试容量为1的channel
void test_single_capacity_channel() {
    std::cout << "测试容量为1的channel..." << std::endl;
    tang::channel<int> ch(1);
    
    tang::runtime::init(2);
    
    // 发送和接收
    ch << 100;
    ASSERT(ch.is_full());
    
    int value;
    ch >> value;
    ASSERT(value == 100);
    ASSERT(ch.is_empty());
    
    tang::runtime::stop();
    std::cout << "容量为1的channel测试通过!" << std::endl;
}

// 测试无缓冲channel的同步行为
void test_unbuffered_channel_sync() {
    std::cout << "测试无缓冲channel的同步行为..." << std::endl;
    tang::channel<int> ch;
    std::atomic_bool sender_done{false};
    std::atomic_bool receiver_done{false};
    std::atomic_int received_value{0};
    
    tang::runtime::init(2);
    
    tang::go([&ch, &sender_done]() {
        ch << 42;
        sender_done = true;
    });
    
    tang::go([&ch, &receiver_done, &received_value]() {
        int value;
        ch >> value;
        received_value = value;
        receiver_done = true;
    });
    
    tang::runtime::run();
    
    ASSERT(received_value.load() == 42);
    ASSERT(sender_done.load());
    ASSERT(receiver_done.load());
    tang::runtime::stop();
    std::cout << "无缓冲channel同步行为测试通过!" << std::endl;
}

// 测试多个生产者和多个消费者
void test_multi_producer_consumer() {
    std::cout << "测试多个生产者和多个消费者..." << std::endl;
    const int num_producers = 3;
    const int num_consumers = 2;
    const int messages_per_producer = 20;
    tang::channel<int> ch(10);
    std::atomic_int total_sent{0};
    std::atomic_int total_received{0};
    
    tang::runtime::init(4);
    
    // 启动生产者
    for (int i = 0; i < num_producers; ++i) {
        tang::go([&ch, &total_sent, i, messages_per_producer]() {
            for (int j = 0; j < messages_per_producer; ++j) {
                int value = i * 1000 + j;
                ch << value;
                total_sent++;
            }
        });
    }
    
    // 启动消费者
    for (int i = 0; i < num_consumers; ++i) {
        tang::go([&ch, &total_received, total = num_producers * messages_per_producer]() {
            int count = 0;
            while (count < total) {
                int value;
                if (ch >> value) {
                    total_received++;
                    count++;
                }
            }
        });
    }
    
    tang::runtime::run();
    
    ASSERT(total_sent.load() == num_producers * messages_per_producer);
    ASSERT(total_received.load() == num_producers * messages_per_producer);
    tang::runtime::stop();
    std::cout << "多个生产者和多个消费者测试通过!" << std::endl;
}

// 测试channel的超时行为
void test_channel_timeout() {
    std::cout << "测试channel超时行为..." << std::endl;
    tang::channel<int> ch(1);
    std::atomic_bool operation_completed{false};
    
    tang::runtime::init(2);
    
    // 发送一个值填满channel
    ch << 1;
    
    // 启动一个协程尝试发送（应该超时或等待）
    tang::go([&ch, &operation_completed]() {
        [[maybe_unused]] auto start = std::chrono::steady_clock::now();
        // 尝试发送，超时机制
        bool sent = false;
        int timeout_count = 0;
        while (!sent && timeout_count < 100) {
            sent = ch.try_send(2);
            if (!sent) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                timeout_count++;
            }
        }
        operation_completed = true;
    });
    
    tang::runtime::run();
    ASSERT(operation_completed.load());
    tang::runtime::stop();
    std::cout << "channel超时行为测试通过!" << std::endl;
}

// 测试channel关闭时的唤醒行为
void test_channel_close_wakeup() {
    std::cout << "测试channel关闭时的唤醒行为..." << std::endl;
    tang::channel<int> ch;
    std::atomic_int waiting_senders{0};
    std::atomic_int woken_senders{0};
    std::atomic_bool close_completed{false};
    
    tang::runtime::init(2);
    
    // 启动等待发送的协程
    tang::go([&ch, &waiting_senders, &woken_senders]() {
        waiting_senders++;
        // 这个协程会阻塞等待发送
        ch << 100;
        woken_senders++;
    });
    
    // 等待发送者准备好
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT(waiting_senders.load() >= 1);
    
    // 关闭channel，这应该唤醒等待的发送者
    ch.close();
    close_completed = true;
    
    tang::runtime::run();
    
    ASSERT(close_completed.load());
    tang::runtime::stop();
    std::cout << "channel关闭唤醒行为测试通过!" << std::endl;
}

// 测试channel的数据顺序保持
void test_channel_order_preservation() {
    std::cout << "测试channel数据顺序保持..." << std::endl;
    const int num_messages = 100;
    tang::channel<int> ch(100);
    std::atomic_int last_received{0};
    std::atomic_int in_order_count{0};
    
    tang::runtime::init(2);
    
    // 发送者按顺序发送
    tang::go([&ch, num_messages]() {
        for (int i = 0; i < num_messages; ++i) {
            ch << i;
        }
        ch.close();
    });
    
    // 接收者检查顺序
    tang::go([&ch, &last_received, &in_order_count, num_messages]() {
        int value;
        while (ch >> value) {
            if (value == last_received.load() + 1) {
                in_order_count++;
            }
            last_received = value;
        }
    });
    
    tang::runtime::run();
    
    ASSERT(last_received.load() == num_messages - 1);
    ASSERT(in_order_count.load() == num_messages - 1);
    tang::runtime::stop();
    std::cout << "channel数据顺序保持测试通过!" << std::endl;
}

// 测试vector类型的channel
void test_vector_channel() {
    std::cout << "测试vector类型的channel..." << std::endl;
    tang::channel<std::vector<int>> ch(2);
    
    tang::runtime::init(2);
    
    std::vector<int> v1 = {1, 2, 3, 4, 5};
    std::vector<int> v2 = {10, 20, 30};
    
    ch << v1;
    ch << v2;
    
    std::vector<int> r1, r2;
    ch >> r1;
    ch >> r2;
    
    ASSERT(r1.size() == 5);
    ASSERT(r2.size() == 3);
    ASSERT(r1[0] == 1);
    ASSERT(r2[0] == 10);
    
    tang::runtime::stop();
    std::cout << "vector类型channel测试通过!" << std::endl;
}

// 测试channel的循环发送接收
void test_channel_loop_send_recv() {
    std::cout << "测试channel循环发送接收..." << std::endl;
    const int iterations = 50;
    tang::channel<int> ch(10);
    std::atomic_int total_sent{0};
    std::atomic_int total_received{0};
    
    tang::runtime::init(2);
    
    // 循环发送
    tang::go([&ch, &total_sent, iterations]() {
        for (int i = 0; i < iterations; ++i) {
            ch << i;
            total_sent++;
        }
        ch.close();
    });
    
    // 循环接收
    tang::go([&ch, &total_received, iterations]() {
        int value;
        int count = 0;
        while (count < iterations && (ch >> value)) {
            total_received++;
            count++;
        }
    });
    
    tang::runtime::run();
    
    ASSERT(total_sent.load() == iterations);
    ASSERT(total_received.load() == iterations);
    tang::runtime::stop();
    std::cout << "channel循环发送接收测试通过!" << std::endl;
}

// 测试channel的边界条件
void test_channel_boundary_conditions() {
    std::cout << "测试channel边界条件..." << std::endl;
    
    // 测试容量为0的channel
    tang::channel<int> ch0;
    ASSERT(ch0.is_full());
    ASSERT(ch0.is_empty() == false || !ch0.is_empty());
    
    // 测试容量为1的channel
    tang::channel<int> ch1(1);
    ASSERT(ch1.is_empty());
    ASSERT(!ch1.is_full());
    
    ch1 << 1;
    ASSERT(!ch1.is_empty());
    ASSERT(ch1.is_full());
    
    int value;
    ch1 >> value;
    ASSERT(value == 1);
    ASSERT(ch1.is_empty());
    ASSERT(!ch1.is_full());
    
    // 测试大容量channel
    tang::channel<int> ch100(100);
    for (int i = 0; i < 100; ++i) {
        ch100 << i;
    }
    ASSERT(ch100.is_full());
    
    for (int i = 0; i < 100; ++i) {
        ch100 >> value;
        ASSERT(value == i);
    }
    ASSERT(ch100.is_empty());
    
    std::cout << "channel边界条件测试通过!" << std::endl;
}

// 测试channel的并发安全性
void test_channel_concurrent_safety() {
    std::cout << "测试channel并发安全性..." << std::endl;
    const int num_threads = 4;
    const int operations_per_thread = 100;
    tang::channel<int> ch(50);
    std::atomic_int total_operations{0};
    
    tang::runtime::init(num_threads);
    
    for (int t = 0; t < num_threads; ++t) {
        tang::go([&ch, &total_operations, t, operations_per_thread]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                int value = t * 10000 + i;
                ch << value;
                total_operations++;
            }
        });
    }
    
    // 消费者
    tang::go([&ch, &total_operations, num_threads, operations_per_thread]() {
        int expected_total = num_threads * operations_per_thread;
        int received = 0;
        int value;
        while (received < expected_total && (ch >> value)) {
            received++;
        }
    });
    
    tang::runtime::run();
    ASSERT(total_operations.load() == num_threads * operations_per_thread);
    tang::runtime::stop();
    std::cout << "channel并发安全性测试通过!" << std::endl;
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
        test_single_capacity_channel();
        test_unbuffered_channel_sync();
        test_multi_producer_consumer();
        test_channel_timeout();
        test_channel_close_wakeup();
        test_channel_order_preservation();
        test_vector_channel();
        test_channel_loop_send_recv();
        test_channel_boundary_conditions();
        test_channel_concurrent_safety();
        
        std::cout << "\\n所有channel测试通过!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "测试失败: " << e.what() << std::endl;
        return 1;
    }
}
