# Tang - C++20 Coroutine Framework

Tang 是一个基于 C++20 协程的轻量级框架，旨在提供 "写起来像 Go，跑起来是 C++" 的编程体验。它实现了 Go 语言的核心并发原语，同时保持 C++ 的高性能和类型安全性。

## 核心特性

### 轻量级任务系统
- 基于 C++20 协程的 `task<T>` 类型
- 支持协程间等待和嵌套
- 自动任务调度

### M:N 线程池调度器
- 工作线程池复用（M:N 模型）
- 工作窃取算法，提高负载均衡
- 支持动态线程数配置

### Go 风格通道
- 类型安全的 `channel<T>` 实现
- 支持缓冲和无缓冲通道
- 异步发送/接收操作
- 支持关闭通道和错误处理

### 选择器机制
- `select` 机制，用于多个通道操作的并发等待
- 支持发送、接收和默认分支

### 语法糖
- `go`/`spawn` 函数，用于启动协程
- `defer` 机制，用于资源清理
- `yield` 函数，用于协程让出 CPU

## 快速开始

### 安装

Tang 是一个头文件库，只需将 `include` 目录添加到您的包含路径中即可使用。

### 构建

#### 依赖
- C++20 兼容的编译器（GCC 10+, Clang 10+, MSVC 2019+）
- CMake 3.16+

#### 构建步骤

```bash
# 克隆仓库
git clone https://github.com/yourusername/tang.git
cd tang

# 创建构建目录
mkdir build
cd build

# 配置 CMake
cmake ..

# 编译
cmake --build .

# 运行示例（可选）
./examples/hello_world
```

## 示例代码

### 基本协程

```cpp
#include <tang/tang.h>
#include <iostream>

tang::task<void> hello() {
    std::cout << "Hello, " << std::flush;
    co_return;
}

tang::task<void> world() {
    std::cout << "World!" << std::endl;
    co_return;
}

tang::task<void> hello_world() {
    co_await hello();
    co_await world();
}

int main() {
    auto task = hello_world();
    task.run();
    tang::runtime::run();
    return 0;
}
```

### 使用 go 函数启动协程

```cpp
#include <tang/tang.h>
#include <iostream>

void say_hello(int id) {
    for (int i = 0; i < 5; ++i) {
        std::cout << "Goroutine " << id << ": Hello " << i << std::endl;
        // 让出 CPU，给其他协程运行机会
        tang::yield();
    }
}

int main() {
    // 启动多个协程
    tang::go(say_hello, 1);
    tang::go(say_hello, 2);
    tang::go(say_hello, 3);
    
    // 运行调度器
    tang::runtime::run();
    return 0;
}
```

### 通道通信

```cpp
#include <tang/tang.h>
#include <iostream>

// 生产者协程
void producer(tang::channel<int>& ch, int count) {
    for (int i = 0; i < count; ++i) {
        co_await ch << i;
        std::cout << "Produced: " << i << std::endl;
    }
    ch.close();
}

// 消费者协程
void consumer(tang::channel<int>& ch) {
    int value;
    while (co_await ch >> value) {
        std::cout << "Consumed: " << value << std::endl;
    }
}

int main() {
    // 创建缓冲通道
    tang::channel<int> ch(3);
    
    // 启动生产者和消费者
    tang::go(producer, ch, 10);
    tang::go(consumer, ch);
    
    // 运行调度器
    tang::runtime::run();
    return 0;
}
```

### 使用 select

```cpp
#include <tang/tang.h>
#include <iostream>

void sender1(tang::channel<int>& ch, int value) {
    // 延迟发送
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    co_await ch << value;
}

void sender2(tang::channel<int>& ch, int value) {
    // 延迟发送
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    co_await ch << value;
}

void selector() {
    tang::channel<int> ch1(0);
    tang::channel<int> ch2(0);
    
    // 启动发送者
    tang::go(sender1, ch1, 10);
    tang::go(sender2, ch2, 20);
    
    int value;
    
    // 使用 select 等待多个通道
    tang::select(
        tang::recv_case(ch1, value, [](int v) {
            std::cout << "Received from ch1: " << v << std::endl;
        }),
        tang::recv_case(ch2, value, [](int v) {
            std::cout << "Received from ch2: " << v << std::endl;
        }),
        tang::default_case([]() {
            std::cout << "Default case executed" << std::endl;
        })
    );
}

int main() {
    tang::go(selector);
    tang::runtime::run();
    return 0;
}
```

### 使用 defer

```cpp
#include <tang/tang.h>
#include <iostream>

void resource_management() {
    std::cout << "Acquiring resource..." << std::endl;
    
    // 使用 defer 确保资源释放
    defer {
        std::cout << "Releasing resource..." << std::endl;
    };
    
    std::cout << "Using resource..." << std::endl;
    
    // 模拟异常情况
    if (true) {
        std::cout << "Exception occurred!" << std::endl;
        return;
    }
    
    std::cout << "Normal exit..." << std::endl;
}

int main() {
    tang::go(resource_management);
    tang::runtime::run();
    return 0;
}
```

## API 参考

### 任务系统

#### `task<T>`
- 表示一个异步任务
- 支持 `co_await` 等待
- 提供 `run()` 方法启动任务

#### `go(func, args...)` / `spawn(func, args...)`
- 启动一个新的协程
- 接受任意可调用对象和参数

#### `yield()`
- 协程让出 CPU
- 允许其他协程运行

### 通道

#### `channel<T>(size_t capacity = 0)`
- 创建通道
- `capacity = 0` 表示无缓冲通道

#### `co_await ch << value`
- 异步发送值到通道

#### `co_await ch >> value`
- 异步从通道接收值

#### `ch.close()`
- 关闭通道

### 选择器

#### `select(cases...)`
- 并发等待多个通道操作

#### `recv_case(ch, value, callback)`
- 接收分支

#### `send_case(ch, value, callback)`
- 发送分支

#### `default_case(callback)`
- 默认分支

### 运行时

#### `runtime::init(size_t num_threads = 0)`
- 初始化运行时
- `num_threads = 0` 表示使用 CPU 核心数

#### `runtime::run()`
- 运行调度器，阻塞直到所有任务完成

#### `runtime::stop()`
- 停止运行时

#### `runtime::num_threads()`
- 获取当前线程数

#### `runtime::set_num_threads(size_t num)`
- 设置线程数

## 构建选项

### CMake 选项

| 选项 | 描述 | 默认值 |
|------|------|--------|
| `CMAKE_CXX_STANDARD` | C++ 标准版本 | 20 |
| `CMAKE_BUILD_TYPE` | 构建类型（Debug/Release） | Debug |

### 编译器支持

- **GCC**: 10+ (需要 `-fcoroutines`) 
- **Clang**: 10+ (需要 `-fcoroutines-ts` 和 libc++)
- **MSVC**: 2019+ (需要 `/std:c++20`)

## 项目结构

```
tang/
├── include/
│   └── tang/
│       ├── tang.h          # 主头文件
│       ├── task.h          # 协程任务系统
│       ├── runtime.h       # 运行时调度器
│       ├── channel.h       # 通道实现
│       ├── select.h        # 选择器机制
│       └── defer.h         # defer 语法糖
├── src/
│   └── runtime.cpp         # 调度器实现
├── examples/               # 示例代码
├── tests/                  # 单元测试
└── CMakeLists.txt          # 构建配置
```

## 性能特点

- **零开销协程**: 基于 C++20 协程，无额外内存分配
- **M:N 调度模型**: 高效的线程复用
- **工作窃取算法**: 动态负载均衡
- **无锁数据结构**: 减少线程竞争
- **类型安全**: 编译时类型检查

## 应用场景

- 高并发服务器
- 异步 I/O 操作
- 并行计算
- 事件驱动编程
- 游戏服务器
- 微服务框架

## 许可证

Tang 采用 MIT 许可证，详见 [LICENSE](LICENSE) 文件。

## 贡献

欢迎提交 Issue 和 Pull Request！

## 联系方式

如有问题或建议，请通过以下方式联系：

- GitHub: [https://github.com/yourusername/tang](https://github.com/yourusername/tang)
- Email: your.email@example.com

## 致谢

- 感谢 Go 语言提供的并发模型灵感
- 感谢 C++20 协程标准的制定者
- 感谢所有贡献者

## 版本历史

### v0.1.0 (2026-01-20)
- 初始版本
- 实现了基本的协程任务系统
- 实现了 M:N 线程池调度器
- 实现了 Go 风格通道
- 实现了 select 机制
- 实现了 defer 语法糖

## 未来计划

- [ ] 完善文档和示例
- [ ] 增加更多测试用例
- [ ] 优化性能
- [ ] 支持更多协程原语
- [ ] 实现定时器
- [ ] 支持网络 I/O 集成
- [ ] 提供更多语言绑定

---

**Write like Go, run as C++**