cmake_minimum_required(VERSION 3.16)

# 主入口文件
set(MAIN_SOURCE "src/main.cpp")

# 前端源文件
set(LEXER_SOURCE "src/frontend/lexer.cpp")
set(PARSER_SOURCE "src/frontend/parser.cpp")
set(SEMANTIC_SOURCE "src/frontend/semantic_analyzer.cpp")

# IR源文件
set(IR_SOURCE "src/ir/ir_gen.cpp")

# 后端源文件
set(BACKEND_SOURCE "src/backend/x86_64_codegen.cpp")

# 编译器可执行文件 - 编译所有模块
add_executable(tangc
    ${MAIN_SOURCE}
    ${LEXER_SOURCE}
    ${PARSER_SOURCE}
    ${SEMANTIC_SOURCE}
    ${IR_SOURCE}
    ${BACKEND_SOURCE}
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

# 语法分析器测试
add_executable(parser_test
    tests/parser_test.cpp
    src/frontend/lexer.cpp
    src/frontend/parser.cpp
)

target_include_directories(parser_test PRIVATE
    include
)

# 语义分析器测试
add_executable(semantic_test
    tests/semantic_test.cpp
    src/frontend/lexer.cpp
    src/frontend/parser.cpp
    src/frontend/semantic_analyzer.cpp
)

target_include_directories(semantic_test PRIVATE
    include
)

# 集成测试
add_executable(integration_test
    tests/integration_test.cpp
    src/frontend/lexer.cpp
    src/frontend/parser.cpp
    src/frontend/semantic_analyzer.cpp
    src/ir/ir_gen.cpp
    src/backend/x86_64_codegen.cpp
)

target_include_directories(integration_test PRIVATE
    include
)

# 运行测试的命令
add_custom_target(run_tests
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/Debug/lexer_test.exe
    DEPENDS lexer_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running lexer tests..."
)

add_custom_target(run_parser_tests
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/Debug/parser_test.exe
    DEPENDS parser_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running parser tests..."
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
