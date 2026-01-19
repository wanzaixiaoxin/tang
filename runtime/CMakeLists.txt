cmake_minimum_required(VERSION 3.16)

# 运行时源文件
set(RUNTIME_SOURCES
    src/tang.c
    src/coroutine/coroutine.cpp
    src/scheduler/scheduler.cpp
    src/io/async_io.cpp
)

# 运行时库
add_library(tang_runtime STATIC
    ${RUNTIME_SOURCES}
)

# 头文件路径
target_include_directories(tang_runtime PUBLIC
    include
)

# 编译选项
if(MSVC)
    # 针对MSVC的特定选项
    target_compile_options(tang_runtime PRIVATE
        /W3 /EHsc
    )
    target_compile_definitions(tang_runtime PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    # 其他编译器选项
    target_compile_options(tang_runtime PRIVATE
        -Wall -Wextra
    )
endif()

# 安装
target_include_directories(tang_runtime PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# 测试例子
add_executable(test_coroutine
    ${CMAKE_SOURCE_DIR}/test/test_coroutine.c
)

# 链接运行时库
target_link_libraries(test_coroutine PRIVATE
    tang_runtime
)

# 头文件路径
target_include_directories(test_coroutine PRIVATE
    include
)

# 编译选项
if(MSVC)
    target_compile_options(test_coroutine PRIVATE
        /W3 /EHsc
    )
    target_compile_definitions(test_coroutine PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(test_coroutine PRIVATE
        -Wall -Wextra
    )
endif()