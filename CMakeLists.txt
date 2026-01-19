cmake_minimum_required(VERSION 3.16)

project(tang LANGUAGES C CXX ASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译器选项
if(MSVC)
    # Windows MSVC编译器选项 - 使用更简单的配置
    add_compile_options(/W3 /EHsc)
else()
    # 其他编译器选项
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0 -Wall -Wextra -Werror)
    else()
        add_compile_options(-O2 -Wall -Wextra)
    endif()
endif()

# 子目录
add_subdirectory(compiler)
add_subdirectory(runtime)
