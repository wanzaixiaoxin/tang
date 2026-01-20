#include <gtest/gtest.h>
#include <tang/tang.h>
#include <vector>
#include <atomic>
#include <stdexcept>
#include <thread>

// 测试基本的select操作
TEST(SelectTest, BasicSelect) {
    // 创建两个channel
    tang::channel<int> ch1;
    tang::channel<int> ch2;
    
    std::atomic_int received = 0;
    std::atomic_int selected_channel = 0;
    
    // 初始化运行时
    tang::runtime::init(2);
    
    // 创建接收协程，使用select等待两个channel
    tang::go([&ch1, &ch2, &received, &selected_channel]() {
        int value;
        
        tang::select(
            tang::case_recv(ch1, value, [&]() {
                received = value;
                selected_channel = 1;
            }),
            tang::case_recv(ch2, value, [&]() {
                received = value;
                selected_channel = 2;
            })
        );
    });
    
    // 创建发送协程，向channel2发送数据
    tang::go([&ch2]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        ch2 << 42;
    });
    
    // 运行调度器
    tang::runtime::run();
    
    // 验证结果
    EXPECT_EQ(received.load(), 42);
    EXPECT_EQ(selected_channel.load(), 2);
    
    // 停止运行时
    tang::runtime::stop();
}

// 测试带有默认case的select
TEST(SelectTest, SelectWithDefault) {
    // 创建一个channel
    tang::channel<int> ch;
    
    std::atomic_bool default_executed = false;
    
    // 初始化运行时
    tang::runtime::init(2);
    
    // 创建协程，使用带有默认case的select
    tang::go([&ch, &default_executed]() {
        int value;
        
        tang::select(
            tang::case_recv(ch, value, [&]() {
                // 这个case不会被执行，因为没有发送数据
            }),
            tang::default_case([&]() {
                default_executed = true;
            })
        );
    });
    
    // 运行调度器
    tang::runtime::run();
    
    // 验证默认case被执行
    EXPECT_TRUE(default_executed.load());
    
    // 停止运行时
    tang::runtime::stop();
}

// 测试多个channel的select
TEST(SelectTest, MultipleChannelsSelect) {
    // 创建三个channel
    tang::channel<int> ch1;
    tang::channel<int> ch2;
    tang::channel<int> ch3;
    
    std::atomic_int received = 0;
    std::atomic_int received_count = 0;
    
    // 初始化运行时
    tang::runtime::init(4);
    
    // 创建接收协程，使用select等待三个channel
    tang::go([&ch1, &ch2, &ch3, &received, &received_count]() {
        const int total = 3;
        while (received_count.load() < total) {
            int value;
            
            tang::select(
                tang::case_recv(ch1, value, [&]() {
                    received += value;
                    received_count++;
                }),
                tang::case_recv(ch2, value, [&]() {
                    received += value;
                    received_count++;
                }),
                tang::case_recv(ch3, value, [&]() {
                    received += value;
                    received_count++;
                })
            );
        }
    });
    
    // 创建三个发送协程，向不同的channel发送数据
    tang::go([&ch1]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        ch1 << 10;
    });
    
    tang::go([&ch2]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        ch2 << 20;
    });
    
    tang::go([&ch3]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        ch3 << 30;
    });
    
    // 运行调度器
    tang::runtime::run();
    
    // 验证结果
    EXPECT_EQ(received.load(), 60);
    EXPECT_EQ(received_count.load(), 3);
    
    // 停止运行时
    tang::runtime::stop();
}

// 测试select的发送case
TEST(SelectTest, SelectSendCase) {
    // 创建两个channel
    tang::channel<int> ch1(1);
    tang::channel<int> ch2(1);
    
    std::atomic_int sent_value = 0;
    std::atomic_int selected_channel = 0;
    
    // 初始化运行时
    tang::runtime::init(2);
    
    // 先向ch1发送一个数据，使其满
    ch1 << 100;
    
    // 创建发送协程，使用select选择发送到哪个channel
    tang::go([&ch1, &ch2, &sent_value, &selected_channel]() {
        int value = 42;
        
        tang::select(
            tang::case_send(ch1, value, [&]() {
                sent_value = value;
                selected_channel = 1;
            }),
            tang::case_send(ch2, value, [&]() {
                sent_value = value;
                selected_channel = 2;
            })
        );
    });
    
    // 创建接收协程，从ch1接收数据，使其可以继续发送
    tang::go([&ch1]() {
        int value;
        ch1 >> value;
    });
    
    // 运行调度器
    tang::runtime::run();
    
    // 验证结果
    EXPECT_EQ(sent_value.load(), 42);
    // 可能选择channel1或channel2，所以使用OR条件
    EXPECT_TRUE(selected_channel.load() == 1 || selected_channel.load() == 2);
    
    // 停止运行时
    tang::runtime::stop();
}

// 测试select的公平性
TEST(SelectTest, SelectFairness) {
    // 创建两个channel
    tang::channel<int> ch1;
    tang::channel<int> ch2;
    
    std::atomic_int ch1_count = 0;
    std::atomic_int ch2_count = 0;
    const int total = 100;
    
    // 初始化运行时
    tang::runtime::init(2);
    
    // 创建接收协程，使用select等待两个channel，统计每个channel被选中的次数
    tang::go([&ch1, &ch2, &ch1_count, &ch2_count]() {
        int value;
        
        for (int i = 0; i < total; ++i) {
            tang::select(
                tang::case_recv(ch1, value, [&]() {
                    ch1_count++;
                }),
                tang::case_recv(ch2, value, [&]() {
                    ch2_count++;
                })
            );
        }
    });
    
    // 创建两个发送协程，同时向两个channel发送数据
    tang::go([&ch1]() {
        for (int i = 0; i < total / 2; ++i) {
            ch1 << i;
        }
    });
    
    tang::go([&ch2]() {
        for (int i = 0; i < total / 2; ++i) {
            ch2 << i;
        }
    });
    
    // 运行调度器
    tang::runtime::run();
    
    // 验证结果：两个channel被选中的次数应该大致相等（考虑到调度的不确定性）
    EXPECT_EQ(ch1_count.load() + ch2_count.load(), total);
    // 允许10%的偏差
    EXPECT_NEAR(ch1_count.load(), total / 2, total * 0.1);
    EXPECT_NEAR(ch2_count.load(), total / 2, total * 0.1);
    
    // 停止运行时
    tang::runtime::stop();
}

// 测试带有多个发送和接收case的select
TEST(SelectTest, MixedSendRecvSelect) {
    // 创建两个channel
    tang::channel<int> ch1(1);
    tang::channel<int> ch2(1);
    
    std::atomic_int received = 0;
    std::atomic_int sent = 0;
    std::atomic_int operation_type = 0;  // 1: recv from ch1, 2: send to ch2
    
    // 初始化运行时
    tang::init(2);
    
    // 先向ch1发送一个数据
    ch1 << 100;
    
    // 创建协程，使用select同时等待接收和发送
    tang::go([&ch1, &ch2, &received, &sent, &operation_type]() {
        int value;
        
        tang::select(
            tang::case_recv(ch1, value, [&]() {
                received = value;
                operation_type = 1;
            }),
            tang::case_send(ch2, 200, [&]() {
                sent = 200;
                operation_type = 2;
            })
        );
    });
    
    // 创建接收协程，从ch2接收数据
    tang::go([&ch2]() {
        int value;
        ch2 >> value;
    });
    
    // 运行调度器
    tang::run();
    
    // 验证结果：应该执行其中一个操作
    if (operation_type.load() == 1) {
        EXPECT_EQ(received.load(), 100);
    } else if (operation_type.load() == 2) {
        EXPECT_EQ(sent.load(), 200);
    } else {
        FAIL() << "No operation was selected";
    }
    
    // 停止运行时
    tang::stop();
}

// 测试关闭channel后select的行为
TEST(SelectTest, SelectOnClosedChannel) {
    // 创建一个channel
    tang::channel<int> ch;
    
    std::atomic_bool executed = false;
    
    // 初始化运行时
    tang::runtime::init(2);
    
    // 关闭channel
    ch.close();
    
    // 创建协程，使用select等待关闭的channel
    tang::go([&ch, &executed]() {
        int value;
        
        tang::select(
            tang::case_recv(ch, value, [&]() {
                executed = true;
            }),
            tang::default_case([&]() {
                executed = true;
            })
        );
    });
    
    // 运行调度器
    tang::runtime::run();
    
    // 验证select执行
    EXPECT_TRUE(executed.load());
    
    // 停止运行时
    tang::runtime::stop();
}
