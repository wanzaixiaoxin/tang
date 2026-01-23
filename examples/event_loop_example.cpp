#include <tang/tang.h>
#include <tang/event_loop.h>
#include <tang/event_source.h>
#include <iostream>
#include <chrono>

// 示例事件源类
class example_event_source : public tang::event_source {
public:
    example_event_source() {
        // 模拟创建一个句柄
        handle_ = this;
    }
    
    ~example_event_source() override = default;
    
    void* get_handle() const override {
        return const_cast<void*>(handle_);
    }
    
    void on_event(uint32_t events) override {
        if (events & static_cast<uint32_t>(tang::event_type::read)) {
            std::cout << "Received read event" << std::endl;
        }
        if (events & static_cast<uint32_t>(tang::event_type::write)) {
            std::cout << "Received write event" << std::endl;
        }
        if (events & static_cast<uint32_t>(tang::event_type::error)) {
            std::cout << "Received error event" << std::endl;
        }
    }
    
private:
    void* handle_;
};

// 示例协程函数
tang::task<void> example_coroutine() {
    std::cout << "Example coroutine started" << std::endl;
    
    // 获取当前运行时的事件循环
    auto& event_loop = tang::runtime::g_scheduler->get_event_loop();
    
    // 创建示例事件源
    example_event_source source;
    
    // 注册事件
    event_loop.register_event(&source, 
        static_cast<uint32_t>(tang::event_type::read) | 
        static_cast<uint32_t>(tang::event_type::write));
    
    // 添加定时器
    auto timer_handle = event_loop.add_timer(std::chrono::seconds(2), []() {
        std::cout << "Timer expired after 2 seconds" << std::endl;
    });
    
    // 模拟一些工作
    co_await tang::task::sleep(std::chrono::seconds(1));
    
    std::cout << "Example coroutine completed" << std::endl;
    
    // 取消定时器
    event_loop.cancel_timer(timer_handle);
    
    // 删除事件
    event_loop.delete_event(&source);
}

int main() {
    try {
        // 初始化运行时
        tang::runtime::init();
        
        // 运行协程
        tang::runtime::schedule(example_coroutine());
        
        // 运行事件循环
        auto& event_loop = tang::runtime::g_scheduler->get_event_loop();
        event_loop.run();
        
        // 停止运行时
        tang::runtime::stop();
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
