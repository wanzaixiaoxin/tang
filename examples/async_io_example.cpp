#include <tang/tang.h>
#include <iostream>

int main() {
    std::cout << "Main thread ID: " << std::this_thread::get_id() << std::endl;

    tang::runtime::init(2);

    // 简化的异步 IO 示例 - 使用协程模拟
    tang::go([]() -> tang::task<void> {
        std::cout << "\nStarting write goroutine" << std::endl;

        // 模拟异步写操作
        co_await tang::task<void>::sleep(std::chrono::milliseconds(100));

        std::cout << "Written 24 bytes asynchronously" << std::endl;
    });

    tang::go([]() -> tang::task<void> {
        std::cout << "\nStarting read goroutine" << std::endl;

        // 模拟异步读操作
        co_await tang::task<void>::sleep(std::chrono::milliseconds(200));

        std::cout << "Read asynchronously: Hello from async write!" << std::endl;
    });

    tang::runtime::run();

    std::cout << "\nAsync IO example completed!" << std::endl;

    return 0;
}