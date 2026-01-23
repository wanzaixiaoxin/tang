# Tang 架构设计文档

## 1. 概述

Tang 是一个基于 C++20 协程的轻量级并发框架,旨在提供"写起来像 Go,跑起来是 C++"的编程体验。框架实现了 Go 语言的核心并发原语,同时保持 C++ 的高性能和类型安全性。

### 1.1 设计目标

- **零开销抽象**: 基于 C++20 标准协程,无额外内存分配开销
- **类型安全**: 完整的编译时类型检查
- **跨平台**: 支持 Windows (MSVC) 和 Linux (GCC/Clang)
- **可扩展**: 模块化设计,易于扩展新功能
- **高性能**: M:N 调度模型,工作窃取算法

### 1.2 核心特性

- `task<T>` 协程任务系统
- M:N 线程池调度器
- Go 风格 `channel<T>` 通道
- `select` 多路复用机制
- `defer` 资源清理语法糖
- `go`/`spawn` 协程启动函数
- 事件循环与定时器支持

## 2. 整体架构

```
┌─────────────────────────────────────────────────────────────────┐
│                        用户应用层                                │
├─────────────────────────────────────────────────────────────────┤
│  task<T>  │  channel<T>  │  select  │  defer  │  go/spawn     │
├─────────────────────────────────────────────────────────────────┤
│                    运行时核心层 (Runtime)                        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐        │
│  │   Scheduler  │  │ Event Loop   │  │  Logger      │        │
│  │  (M:N 调度)  │  │  (定时器/I/O) │  │  (日志)      │        │
│  └──────────────┘  └──────────────┘  └──────────────┘        │
├─────────────────────────────────────────────────────────────────┤
│                    平台抽象层                                    │
│  ┌──────────────┐  ┌──────────────┐                           │
│  │  Windows     │  │   Linux      │                           │
│  │  (IOCP)      │  │  (epoll)     │                           │
│  └──────────────┘  └──────────────┘                           │
├─────────────────────────────────────────────────────────────────┤
│                    操作系统层                                    │
│          C++20 协程标准库 │ 线程 API │ 系统调用               │
└─────────────────────────────────────────────────────────────────┘
```

## 3. 核心模块设计

### 3.1 任务系统 (Task System)

#### 3.1.1 `task<T>` 类

基于 C++20 协程的异步任务类型,提供协程的创建、等待和结果获取机制。

**核心组件:**

```cpp
template <typename T = void>
class task {
    struct promise_type {
        std::optional<T> result;           // 任务返回值
        std::exception_ptr exception;     // 异常处理
        std::coroutine_handle<> continuation;  // 续体(调用者)
        
        task get_return_object();
        std::suspend_always initial_suspend();  // 创建时挂起
        final_awaiter final_suspend();          // 完成时恢复调用者
        void return_value(T value);             // void特化用return_void
        void unhandled_exception();             // 异常捕获
    };
};
```

**执行流程:**

```
协程创建
   ↓
initial_suspend() - 创建时立即挂起
   ↓
用户调用 task.run() 或 co_await task
   ↓
await_suspend() - 将协程句柄提交到调度器
   ↓
调度器恢复执行协程体
   ↓
协程执行过程中可能再次挂起
   ↓
final_suspend() - 完成后恢复continuation
```

#### 3.1.2 `go` / `spawn` 函数

语法糖函数,用于快速启动协程并自动调度执行。

```cpp
template <typename F, typename... Args>
auto go(F&& f, Args&&... args) {
    // 1. 推导返回类型
    using result_type = decltype(std::declval<F&>()(std::declval<Args>()...));
    
    // 2. 创建协程
    auto task = go_helper<result_type>::create(std::forward<F>(f), ...);
    
    // 3. 立即调度执行
    task.run();
    
    return task;
}
```

### 3.2 运行时调度器 (Runtime Scheduler)

#### 3.2.1 调度器架构

采用 **M:N 调度模型**,将多个协程映射到多个工作线程上执行。

```cpp
class scheduler {
    std::vector<std::thread> threads_;              // 工作线程池
    std::list<std::coroutine_handle<>> task_queue_;  // 任务队列(FIFO)
    std::mutex queue_mutex_;                          // 队列互斥锁
    std::atomic_bool running_;                        // 运行状态
    std::unique_ptr<event_loop> event_loop_;          // 事件循环
};
```

#### 3.2.2 调度策略

**FIFO 调度:**
- 使用 `std::list` 维护任务队列
- 按提交顺序执行,保证公平性
- 工作线程从队首取任务

**负载均衡:**
- 当前实现为简单的 FIFO 队列,所有工作线程共享同一个队列
- 未来计划实现工作窃取算法(work-stealing),每个线程维护本地队列

#### 3.2.3 执行流程

```
用户调用 runtime::schedule(handle)
   ↓
协程句柄加入全局任务队列
   ↓
工作线程从队列取任务
   ↓
检查协程是否已完成
   ↓
resume() 恢复协程执行
   ↓
协程执行完成?
    ├─ 是 → 销毁句柄
    └─ 否 → 重新调度到队列
```

### 3.3 通道系统 (Channel System)

#### 3.3.1 `channel<T>` 类

实现 Go 风格的通道,用于协程间通信。

**核心特性:**
- 支持缓冲和无缓冲通道
- 类型安全的发送/接收
- 自动阻塞机制
- 通道关闭与错误处理

```cpp
template <typename T>
class channel {
    size_t capacity_;                              // 容量(0=无缓冲)
    std::list<T> buffer_;                          // 缓冲区(FIFO)
    mutable std::mutex mutex_;                      // 互斥锁
    std::atomic<bool> closed_{false};               // 关闭状态
    
    // 等待者列表
    std::list<std::unique_ptr<send_waiter<T>>> send_waiters_;
    std::list<std::unique_ptr<recv_waiter<T>>> recv_waiters_;
};
```

#### 3.3.2 发送/接收机制

**发送流程:**

```
ch << value 或 co_await ch << value
   ↓
try_send() 尝试直接发送
   ↓
┌─────────────────────────────┐
│ 检查接收者队列               │
│ 有接收者?                    │
│   ├─ 是 → 直接传递给接收者   │
│   └─ 否 → 检查缓冲区         │
├─────────────────────────────┤
│ 检查缓冲区                   │
│ 未满?                        │
│   ├─ 是 → 写入缓冲区         │
│   └─ 否 → 注册为发送等待者   │
└─────────────────────────────┘
```

**接收流程:**

```
ch >> value 或 co_await ch >> value
   ↓
try_recv() 尝试直接接收
   ↓
┌─────────────────────────────┐
│ 检查缓冲区                   │
│ 有数据?                      │
│   ├─ 是 → 从缓冲区取出       │
│   │       → 唤醒发送等待者   │
│   └─ 否 → 检查发送者队列     │
├─────────────────────────────┤
│ 检查发送者队列               │
│ 有发送者?                    │
│   ├─ 是 → 直接获取数据       │
│   └─ 否 → 注册为接收等待者   │
└─────────────────────────────┘
```

#### 3.3.3 等待者机制

使用 `coro_waiter` 基类实现等待者注册与唤醒:

```cpp
class coro_waiter {
public:
    virtual void resume() = 0;  // 唤醒协程
};

class send_waiter : public coro_waiter {
    std::coroutine_handle<> handle_;
    T value_;  // 待发送的值
};

class recv_waiter : public coro_waiter {
    std::coroutine_handle<> handle_;
    T* value_ptr_;  // 接收值的指针
};
```

### 3.4 选择器 (Select)

#### 3.4.1 多路复用机制

`select` 允许同时等待多个通道操作,随机选择可执行的一个。

```cpp
void select(Cases&&... cases) {
    // 1. 收集所有 case
    std::vector<std::unique_ptr<select_case>> case_ptrs;
    
    // 2. 随机打乱(保证公平性)
    std::shuffle(case_ptrs.begin(), case_ptrs.end());
    
    // 3. 尝试执行立即可用的 case
    for (auto& case_ptr : case_ptrs) {
        if (case_ptr->try_ready() && case_ptr->try_execute()) {
            return;
        }
    }
    
    // 4. 有 default case 则执行
    // 5. 否则阻塞等待直到有 case 可执行
}
```

#### 3.4.2 Select Case 类型

- **recv_case**: 接收分支,等待通道可读
- **send_case**: 发送分支,等待通道可写
- **default_case**: 默认分支,无阻塞立即执行

#### 3.4.3 公平性保证

使用 `std::shuffle` 随机打乱 case 顺序,避免某些分支饥饿。

### 3.5 事件循环 (Event Loop)

#### 3.5.1 架构设计

基于平台抽象层实现跨平台的事件循环:

```cpp
class event_loop {
    std::unique_ptr<event_poller> poller_;      // 平台相关轮询器
    std::atomic<bool> running_;
    
    // 定时器管理
    uint64_t next_timer_id_;
    std::unordered_map<timer_handle,
        std::pair<std::chrono::steady_clock::time_point,
                  timer_callback>> timers_;
};
```

#### 3.5.2 平台抽象层

**Windows (IOCP):**
```cpp
class event_poller_win : public event_poller {
    HANDLE io_port_;  // IOCP 端口
    // 实现基于 IOCP 的事件轮询
};
```

**Linux (epoll):**
```cpp
class event_poller_linux : public event_poller {
    int epoll_fd_;  // epoll 文件描述符
    // 实现基于 epoll 的事件轮询
};
```

#### 3.5.3 定时器管理

使用 `unordered_map` 存储定时器,以到期时间为键:

```cpp
std::chrono::milliseconds handle_timers() {
    auto now = std::chrono::steady_clock::now();
    std::chrono::milliseconds next_timeout = std::chrono::milliseconds::max();
    
    for (auto it = timers_.begin(); it != timers_.end();) {
        if (expire_time <= now) {
            callback();  // 执行回调
            it = timers_.erase(it);
        } else {
            // 计算最小超时时间
            next_timeout = min(next_timeout, expire_time - now);
            ++it;
        }
    }
    
    return next_timeout;  // 返回下一次轮询超时
}
```

### 3.6 Defer 机制

#### 3.6.1 RAII 实现原理

利用 RAII (Resource Acquisition Is Initialization) 模式实现延迟执行:

```cpp
class defer_guard {
    std::function<void()> func_;
    
    ~defer_guard() {
        if (func_) {
            func_();  // 析构时执行清理函数
        }
    }
};

#define defer \
    tang::defer_guard TANG_DEFER_VAR_NAME(__LINE__) = \
        tang::defer_guard([&]()  // 立即创建临时对象
#define end_defer );            // 语句结束
```

#### 3.6.2 使用示例

```cpp
void resource_management() {
    std::cout << "Acquiring resource..." << std::endl;
    
    defer {
        std::cout << "Releasing resource..." << std::endl;  // 作用域结束时执行
    } end_defer
    
    std::cout << "Using resource..." << std::endl;
    
    if (error) {
        return;  // defer 代码仍会执行
    }
}
```

## 4. 内存管理

### 4.1 协程帧管理

C++20 协程使用编译器自动生成的协程帧,由 `promise_type` 管理生命周期:

```
协程创建
   ↓
编译器分配协程帧(堆或优化到栈)
   ↓
promise_type.get_return_object() 返回 task 对象
   ↓
用户持有 task 对象(移动语义)
   ↓
task 析构时调用 handle.destroy() 销毁协程帧
```

### 4.2 零拷贝优化

- **移动语义**: `task` 和 `channel` 禁止拷贝,只允许移动
- **智能指针**: 使用 `unique_ptr` 管理等待者对象
- **引用传递**: 通道通信时优先使用 `T&&` 避免拷贝

## 5. 并发模型

### 5.1 M:N 调度

```
协程1 协程2 协程3 协程4 ... 协程N
   ↓     ↓     ↓     ↓         ↓
┌───────────────────────────────────┐
│       全局任务队列 (FIFO)         │
└───────────────────────────────────┘
         ↓        ↓        ↓
     线程1     线程2     线程3
```

**优势:**
- 减少线程创建开销
- 避免上下文切换(协程切换在用户态)
- 提高缓存局部性

### 5.2 同步原语

- **Mutex**: 保护共享数据(task_queue_, channel buffer_)
- **Atomic**: 运行状态标志(running_, closed_)
- **Condition Variable**: 等待任务完成(completion_cv_)

### 5.3 异常处理

- 协程内异常由 `unhandled_exception()` 捕获
- 异常存储在 `exception_ptr` 中
- `await_resume()` 时重新抛出异常

## 6. 日志系统

### 6.1 模块化设计

```cpp
namespace logger {
    extern Logger runtime;   // 运行时日志
    extern Logger channel;   // 通道日志
    extern Logger task;      // 任务日志
    extern Logger test;      // 测试日志
    extern Logger example;   // 示例日志
}
```

### 6.2 日志级别

- `DEBUG`: 调试信息(可编译时关闭)
- `INFO`: 一般信息
- `WARN`: 警告信息
- `ERROR`: 错误信息

### 6.3 线程安全

使用静态 `mutex` 保证多线程环境下的日志输出安全。

## 7. 平台差异

### 7.1 编译器支持

| 编译器  | 最低版本  | 编译选项        |
|--------|----------|----------------|
| GCC    | 10+      | `-fcoroutines` |
| Clang  | 10+      | `-fcoroutines-ts` |
| MSVC   | 2019+    | `/await:strict` |

### 7.2 事件轮询

| 平台   | 实现方式   | 源文件              |
|--------|-----------|--------------------|
| Windows | IOCP     | event_poller_win.cpp |
| Linux   | epoll    | event_poller_linux.cpp |

### 7.3 时间函数

Windows 使用 `localtime_s`,Linux 使用 `localtime` 保证线程安全。

## 8. 性能优化

### 8.1 已实现的优化

1. **零开销协程**: 基于 C++20 标准库
2. **移动语义**: 避免不必要的拷贝
3. **FIFO 调度**: 简单高效的任务队列
4. **定时器优化**: 批量处理到期定时器

### 8.2 未来优化方向

1. **工作窃取算法**: 每个线程维护本地队列,减少锁竞争
2. **无锁队列**: 使用 `atomic` 实现无锁任务队列
3. **协程池**: 复用协程帧,减少堆分配
4. **内存池**: 专用分配器管理等待者对象
5. **编译器优化**: `[[likely]]`/`[[unlikely]]` 提示分支预测

## 9. 测试策略

### 9.1 测试结构

```
tests/
├── test_framework.h      // 测试框架
├── test_framework.cpp    // 框架实现
├── simple_test.cpp        // 基础测试
├── task_test.cpp          // 任务系统测试
├── channel_test.cpp       // 通道测试
├── select_test.cpp        // 选择器测试
├── runtime_test.cpp       // 运行时测试
└── integration_test.cpp    // 集成测试
```

### 9.2 测试覆盖

- **单元测试**: 测试单个组件的功能
- **集成测试**: 测试组件间的协作
- **并发测试**: 测试多线程环境下的正确性
- **压力测试**: 测试性能和稳定性

## 10. 扩展性设计

### 10.1 插件机制

通过继承 `coro_waiter` 和 `select_case` 可扩展新的等待和选择机制。

### 10.2 自定义调度器

`scheduler` 类设计为可继承,用户可实现自定义调度策略。

### 10.3 事件源扩展

通过继承 `event_source` 可支持新的 I/O 事件类型。

## 11. 设计模式

| 模式         | 应用位置              | 说明                     |
|-------------|----------------------|-------------------------|
| RAII        | `defer_guard`        | 资源自动释放             |
| Builder     | `select`             | 灵活组合多个 case        |
| Strategy    | `event_poller`       | 平台抽象(多态)           |
| Observer    | `event_source`       | 事件监听机制             |
| Iterator    | `std::list`          | FIFO 遍历                |

## 12. 依赖关系

```
tang.h (主头文件)
├── task.h (协程任务)
│   ├── logger.h
│   └── runtime.h (forward declaration)
├── runtime.h (运行时)
│   ├── event_loop.h
│   └── coroutine (标准库)
├── channel.h (通道)
│   ├── logger.h
│   └── runtime.h (forward declaration)
├── select.h (选择器)
│   └── channel.h
└── defer.h (延迟执行)
    └── functional (标准库)
```

## 13. 使用建议

### 13.1 最佳实践

1. **优先使用 channel 而非共享内存**: 避免锁竞争
2. **合理设置缓冲区大小**: 权衡内存和性能
3. **使用 defer 管理资源**: 确保异常安全
4. **避免长时间阻塞**: 协程应快速让出 CPU

### 13.2 注意事项

1. **协程帧大小**: 注意栈帧大小,避免大对象
2. **生命周期**: 确保 channel 引用在协程内有效
3. **异常处理**: 协程内异常不会传播到调用者,需手动检查
4. **循环依赖**: 避免 channel 之间的循环等待导致死锁

## 14. 版本历史

### v0.1.0 (2026-01-20)

- 实现基本的协程任务系统
- 实现 M:N 线程池调度器
- 实现 Go 风格通道
- 实现 select 机制
- 实现 defer 语法糖
- 实现事件循环与定时器
- 实现跨平台支持(Windows/Linux)
