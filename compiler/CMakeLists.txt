cmake_minimum_required(VERSION 3.16)

# 主入口文件（使用简化版本）
set(MAIN_SOURCE "src/simple_main.cpp")

# 前端源文件
set(LEXER_SOURCE "src/frontend/lexer.cpp")
set(SEMANTIC_SOURCE "src/frontend/semantic_analyzer.cpp")

# 编译器可执行文件 - 编译简化版本
add_executable(tangc
    ${MAIN_SOURCE}
    ${LEXER_SOURCE}
    ${SEMANTIC_SOURCE}
)

# 头文件路径
target_include_directories(tangc PRIVATE
    include
)

# 运行时库依赖
target_link_libraries(tangc PRIVATE
    tang_runtime
)

# 安装
target_include_directories(tangc PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
