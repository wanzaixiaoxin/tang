# Tang 编程语言

Tang 是一门实验性的编译型编程语言，原生支持异步IO和协程，采用 N:M 用户态线程模型。

## 核心特性

- **原生异步IO**：默认所有操作都是异步的，无需额外关键字
- **原生协程**：轻量级用户态线程，上下文切换使用汇编实现
- **N:M 线程模型**：用户态线程与内核线程的灵活映射

## 项目结构

```
tang/
├── compiler/          # 编译器
│   ├── include/       # 头文件
│   │   ├── lexer.h
│   │   ├── parser.h
│   │   ├── ast.h
│   │   ├── ir.h
│   │   ├── semantic_analyzer.h
│   │   └── x86_64_codegen.h
│   ├── src/           # 源代码
│   ├── tests/         # 测试
│   └── CMakeLists.txt
├── runtime/           # 运行时库
│   ├── src/           # 运行时代码
│   │   ├── coroutine/     # 协程实现
│   │   ├── scheduler/     # 调度器
│   │   ├── io/            # 异步IO
│   │   └── thread/        # 线程管理
│   ├── include/       # 头文件
│   └── CMakeLists.txt
├── stdlib/            # 标准库
├── test/              # 测试用例
├── CMakeLists.txt     # 顶层 CMake 配置
└── .gitignore
```

## 编译流程

源代码 → 词法分析 → 语法分析 → 语义分析 → IR生成 → 机器码生成

## 构建要求

- CMake 3.16+
- C++20 编译器
- 汇编器（用于上下文切换）
- Linux x86_64 Windows

## 构建方法

```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译
cmake --build .

# 运行测试
ctest
```

## 语言语法示例

### 变量和函数

```tang
fn add(a: int, b: int) -> int {
    return a + b;
}

let x: int = 10;
let y: int = 20;
let result: int = add(x, y);
```

### 异步函数

默认所有函数都是异步的：

```tang
async fn fetch_data(url: &str) -> Data {
    let response = http_get(url);
    return response.body;
}
```

### 同步函数

需要显式标注同步操作：

```tang
sync fn read_file_sync(path: &str) -> Result<String, Error> {
    let file = open(path);
    return file.read_all();
}
```

## 许可证

本项目仅供学习研究使用。
