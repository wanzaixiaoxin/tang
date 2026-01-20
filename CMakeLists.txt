cmake_minimum_required(VERSION 3.16)

project(tang_cpp20_coroutine_framework VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
if(MSVC)
    # Windows编译选项
    add_compile_options(/W4 /permissive- /Zc:__cplusplus /wd4819)
else()
    # Linux/macOS编译选项
    add_compile_options(-Wall -Wextra -Wpedantic -fcoroutines)
    add_link_options(-fcoroutines)
endif()

# 包含目录
include_directories(include)

# 源文件
set(SOURCE_FILES
    src/runtime.cpp
)

# 创建库
add_library(tang STATIC ${SOURCE_FILES})

# 安装配置
install(TARGETS tang DESTINATION lib)
install(DIRECTORY include/tang DESTINATION include)

# 示例代码
add_subdirectory(examples)

# 单元测试
enable_testing()
add_subdirectory(tests)
