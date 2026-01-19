cmake_minimum_required(VERSION 3.16)

# 主入口文件
set(MAIN_SOURCE "src/main.cpp")

# 词法分析器源文件
set(LEXER_SOURCE "src/frontend/lexer.cpp")

# 编译器可执行文件 - 只编译主入口和词法分析器
add_executable(tangc
    ${MAIN_SOURCE}
    ${LEXER_SOURCE}
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

# 测试
add_executable(lexer_test
    tests/lexer_test.cpp
    src/frontend/lexer.cpp
)

target_include_directories(lexer_test PRIVATE
    include
)

# 运行测试的命令
add_custom_target(run_tests
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/Debug/lexer_test.exe
    DEPENDS lexer_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running lexer tests..."
)

# 简单的词法分析器测试
target_link_libraries(tangc PRIVATE tang_runtime)
add_executable(simple_lexer_test
    tests/simple_lexer_test.cpp
    src/frontend/lexer.cpp
)

target_include_directories(simple_lexer_test PRIVATE
    include
)

target_link_libraries(simple_lexer_test PRIVATE tang_runtime)
