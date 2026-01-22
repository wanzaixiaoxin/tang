#include <iostream>
#include <cassert>
#include <tang/tang.h>
#include <vector>
#include <atomic>
#include <stdexcept>
#include <thread>
#include <functional>

// 简单的断言宏
#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion failed: " << #condition << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::terminate(); \
        } \
    } while(0)

// 测试基本的select操作
tang::task<void> test_basic_select() {
    std::cout << "测试基本select操作..." << std::endl;
    // 创建两个channel
    tang::channel<int> ch1;
    tang::channel<int> ch2;
    
    std::atomic_int received = 0;
    std::atomic_int selected_channel = 0;
    
    // 创建发送协程，向channel2发送数据
    tang::go([&ch2]() -> tang::task<void> {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        ch2 << 42;
        co_return;
    });
    
    // 使用select等待两个channel
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
    
    // 验证结果
    ASSERT(received.load() == 42);
    ASSERT(selected_channel.load() == 2);
    
    std::cout << "基本select操作测试通过!" << std::endl;
    co_return;
}

// 测试带有默认case的select
tang::task<void> test_select_with_default() {
    std::cout << "测试带有默认case的select..." << std::endl;
    // 创建一个channel
    tang::channel<int> ch;
    
    std::atomic_bool default_executed = false;
    
    int value;
    
    tang::select(
        tang::case_recv(ch, value, [&]() {
            // 这个case不会被执行，因为没有发送数据
        }),
        tang::default_case([&]() {
            default_executed = true;
        })
    );
    
    // 验证默认case被执行
    ASSERT(default_executed.load());
    
    std::cout << "带有默认case的select测试通过!" << std::endl;
    co_return;
}

// 测试多个channel的select
tang::task<void> test_multiple_channels_select() {
    std::cout << "测试多个channel的select..." << std::endl;
    // 创建三个channel
    tang::channel<int> ch1;
    tang::channel<int> ch2;
    tang::channel<int> ch3;
    
    std::atomic_int received = 0;
    std::atomic_int received_count = 0;
    
    // 创建三个发送协程，向不同的channel发送数据
    tang::go([&ch1]() -> tang::task<void> {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        ch1 << 10;
        co_return;
    });
    
    tang::go([&ch2]() -> tang::task<void> {
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        ch2 << 20;
        co_return;
    });
    
    tang::go([&ch3]() -> tang::task<void> {
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        ch3 << 30;
        co_return;
    });
    
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
    
    // 验证结果
    ASSERT(received.load() == 60);
    ASSERT(received_count.load() == 3);
    
    std::cout << "多个channel的select测试通过!" << std::endl;
    co_return;
}

// 测试select的发送case
tang::task<void> test_select_send_case() {
    std::cout << "测试select的发送case..." << std::endl;
    // 创建两个channel
    tang::channel<int> ch1(1);
    tang::channel<int> ch2(1);
    
    std::atomic_int sent_value = 0;
    std::atomic_int selected_channel = 0;
    
    // 先向ch1发送一个数据，使其满
    ch1 << 100;
    
    // 创建接收协程，从ch1接收数据，使其可以继续发送
    tang::go([&ch1]() -> tang::task<void> {
        int value;
        ch1 >> value;
        co_return;
    });
    
    // 使用select选择发送到哪个channel
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
    
    // 验证结果
    ASSERT(sent_value.load() == 42);
    // 可能选择channel1或channel2，所以使用OR条件
    ASSERT(selected_channel.load() == 1 || selected_channel.load() == 2);
    
    std::cout << "select的发送case测试通过!" << std::endl;
    co_return;
}

// 测试select的公平性
tang::task<void> test_select_fairness() {
    std::cout << "测试select的公平性..." << std::endl;
    // 创建两个channel
    tang::channel<int> ch1;
    tang::channel<int> ch2;
    
    std::atomic_int ch1_count = 0;
    std::atomic_int ch2_count = 0;
    const int total = 100; // 增加测试次数以更好地验证公平性
    
    // 创建两个发送协程，同时向两个channel发送数据
    tang::go([&ch1, total]() -> tang::task<void> {
        for (int i = 0; i < total / 2; ++i) {
            ch1 << i;
        }
        co_return;
    });
    
    tang::go([&ch2, total]() -> tang::task<void> {
        for (int i = 0; i < total / 2; ++i) {
            ch2 << i;
        }
        co_return;
    });
    
    // 使用select等待两个channel，统计每个channel被选中的次数
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
    
    // 验证结果：两个channel被选中的次数应该大致相等（考虑到调度的不确定性）
    ASSERT(ch1_count.load() + ch2_count.load() == total);
    
    // 计算偏差，确保公平性（允许10%的偏差）
    int expected = total / 2;
    int diff = std::abs(static_cast<int>(ch1_count.load()) - expected);
    double tolerance = expected * 0.1;
    ASSERT(diff <= tolerance);
    
    std::cout << "select的公平性测试通过!" << std::endl;
    std::cout << "ch1_count: " << ch1_count.load() << ", ch2_count: " << ch2_count.load() << std::endl;
    co_return;
}

// 测试select与channel关闭
tang::task<void> test_select_with_channel_close() {
    std::cout << "测试select与channel关闭..." << std::endl;
    tang::channel<int> ch1;
    tang::channel<int> ch2;
    std::atomic_int received_value = 0;
    std::atomic_bool close_handled = false;
    
    tang::go([&ch1]() -> tang::task<void> {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        ch1 << 100;
        co_return;
    });
    
    tang::go([&ch2, &close_handled]() -> tang::task<void> {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ch2.close();
        close_handled = true;
        co_return;
    });
    
    int value;
    tang::select(
        tang::case_recv(ch1, value, [&]() {
            received_value = value;
        }),
        tang::case_recv(ch2, value, [&]() {
            // 关闭的channel不应被选中
            received_value = -1;
        })
    );
    
    ASSERT(received_value.load() == 100);
    ASSERT(close_handled.load());
    
    std::cout << "select与channel关闭测试通过!" << std::endl;
    co_return;
}

// 测试select的超时行为
tang::task<void> test_select_timeout() {
    std::cout << "测试select超时行为..." << std::endl;
    tang::channel<int> ch;
    std::atomic_bool default_executed = false;
    int timeout_count = 0;
    int value = 0;
    
    for (int i = 0; i < 5; ++i) {
        tang::select(
            tang::case_recv(ch, value, [&]() {}),
            tang::default_case([&]() {
                default_executed = true;
                timeout_count++;
            })
        );
    }
    
    ASSERT(timeout_count == 5);
    ASSERT(default_executed.load());
    
    std::cout << "select超时行为测试通过!" << std::endl;
    co_return;
}

// 测试大量channel的select
tang::task<void> test_select_many_channels() {
    std::cout << "测试大量channel的select..." << std::endl;
    // 不使用vector，直接使用多个独立的channel变量
    std::atomic_int selected_channel = -1;
    
    // 使用独立的channel数组代替vector
    tang::channel<int> ch1, ch2, ch3, ch4, ch5, ch6, ch7, ch8, ch9, ch10;
    tang::channel<int>* channels[] = {&ch1, &ch2, &ch3, &ch4, &ch5, &ch6, &ch7, &ch8, &ch9, &ch10};
    
    // 随机选择一个channel发送数据
    int target_channel = 5;
    tang::go([&channels, target_channel]() -> tang::task<void> {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        *channels[target_channel] << 42;
        co_return;
    });
    
    int value = 0;
    tang::select(
        tang::case_recv(*channels[0], value, [&]() { selected_channel = 0; }),
        tang::case_recv(*channels[1], value, [&]() { selected_channel = 1; }),
        tang::case_recv(*channels[2], value, [&]() { selected_channel = 2; }),
        tang::case_recv(*channels[3], value, [&]() { selected_channel = 3; }),
        tang::case_recv(*channels[4], value, [&]() { selected_channel = 4; }),
        tang::case_recv(*channels[5], value, [&]() { selected_channel = 5; }),
        tang::case_recv(*channels[6], value, [&]() { selected_channel = 6; }),
        tang::case_recv(*channels[7], value, [&]() { selected_channel = 7; }),
        tang::case_recv(*channels[8], value, [&]() { selected_channel = 8; }),
        tang::case_recv(*channels[9], value, [&]() { selected_channel = 9; })
    );
    
    ASSERT(selected_channel.load() == target_channel);
    ASSERT(value == 42);
    
    std::cout << "大量channel的select测试通过!" << std::endl;
    co_return;
}

// 测试select中的异常处理
tang::task<void> test_select_exception_handling() {
    std::cout << "测试select中的异常处理..." << std::endl;
    tang::channel<int> ch;
    std::atomic_bool exception_caught = false;
    std::atomic_bool callback_executed = false;
    
    tang::go([&ch]() -> tang::task<void> {
        ch << 100;
        co_return;
    });
    
    int value;
    try {
        tang::select(
            tang::case_recv(ch, value, [&]() {
                if (value > 50) {
                    throw std::runtime_error("Value too large");
                }
                callback_executed = true;
            })
        );
    } catch (const std::exception& e) {
        exception_caught = true;
    }
    
    ASSERT(exception_caught.load());
    
    std::cout << "select中的异常处理测试通过!" << std::endl;
    co_return;
}

// 测试select的混合recv和send
tang::task<void> test_select_mixed_recv_send() {
    std::cout << "测试select的混合recv和send..." << std::endl;
    tang::channel<int> ch1(1);
    tang::channel<int> ch2;
    std::atomic_int operation_type = -1;
    
    ch1 << 10;
    
    tang::go([&ch2]() -> tang::task<void> {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ch2 << 20;
        co_return;
    });
    
    int value;
    tang::select(
        tang::case_recv(ch1, value, [&]() {
            operation_type = 0;
        }),
        tang::case_send(ch1, 30, [&]() {
            operation_type = 1;
        }),
        tang::case_recv(ch2, value, [&]() {
            operation_type = 2;
        })
    );
    
    ASSERT(operation_type.load() >= 0 && operation_type.load() <= 2);
    
    std::cout << "select的混合recv和send测试通过!" << std::endl;
    co_return;
}

// 测试select的递归使用
tang::task<void> test_select_nested() {
    std::cout << "测试select的嵌套使用..." << std::endl;
    tang::channel<int> ch1;
    tang::channel<int> ch2;
    std::atomic_int result = 0;
    
    tang::go([&ch1]() -> tang::task<void> {
        ch1 << 1;
        co_return;
    });
    
    tang::go([&ch2]() -> tang::task<void> {
        ch2 << 2;
        co_return;
    });
    
    int value1, value2;
    tang::select(
        tang::case_recv(ch1, value1, [&]() {
            result = value1;
            tang::select(
                tang::case_recv(ch2, value2, [&]() {
                    result += value2;
                }),
                tang::default_case([&]() {})
            );
        }),
        tang::case_recv(ch2, value2, [&]() {
            result = value2;
        })
    );
    
    ASSERT(result.load() == 3);
    
    std::cout << "select的嵌套使用测试通过!" << std::endl;
    co_return;
}

// 测试select的多次选择
tang::task<void> test_select_multiple_selections() {
    std::cout << "测试select的多次选择..." << std::endl;
    tang::channel<int> ch1;
    tang::channel<int> ch2;
    std::atomic_int select_count = 0;
    std::atomic_int sum = 0;
    
    tang::go([&ch1, &ch2]() -> tang::task<void> {
        for (int i = 0; i < 5; ++i) {
            ch1 << i;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        ch1.close();
        ch2.close();
        co_return;
    });
    
    int value;
    while (true) {
        tang::select(
            tang::case_recv(ch1, value, [&]() {
                select_count++;
                sum += value;
            }),
            tang::default_case([&]() {
                if (select_count.load() >= 5) {
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            })
        );
        if (select_count.load() >= 5) {
            break;
        }
    }
    
    ASSERT(select_count.load() == 5);
    ASSERT(sum.load() == 10);
    
    std::cout << "select的多次选择测试通过!" << std::endl;
    co_return;
}

// 测试运行器
tang::task<void> run_tests() {
    co_await test_basic_select();
    co_await test_select_with_default();
    co_await test_multiple_channels_select();
    co_await test_select_send_case();
    co_await test_select_fairness();
    co_await test_select_with_channel_close();
    co_await test_select_timeout();
    co_await test_select_many_channels();
    co_await test_select_exception_handling();
    co_await test_select_mixed_recv_send();
    co_await test_select_nested();
    co_await test_select_multiple_selections();
    co_return;
}

// 主函数
int main() {
    std::cout << "开始运行select测试..." << std::endl;
    
    try {
        // 初始化运行时
        tang::runtime::init(2);
        
        // 运行所有测试
        auto test_task = run_tests();
        test_task.run();
        
        // 运行调度器
        tang::runtime::run();
        
        std::cout << "\n所有select测试通过!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "测试失败: " << e.what() << std::endl;
        return 1;
    }
}