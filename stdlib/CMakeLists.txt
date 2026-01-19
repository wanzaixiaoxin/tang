cmake_minimum_required(VERSION 3.16)

# 标准库源文件
file(GLOB STDLIB_SOURCES "src/*.cpp")

# 标准库
add_library(tang_stdlib STATIC
    ${STDLIB_SOURCES}
)

# 头文件路径
target_include_directories(tang_stdlib PUBLIC
    include
)

# 依赖运行时库
target_link_libraries(tang_stdlib PUBLIC
    tang_runtime
)

# 编译选项
target_compile_options(tang_stdlib PRIVATE
    -fno-exceptions
    -fno-rtti
)

# 安装
target_include_directories(tang_stdlib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
