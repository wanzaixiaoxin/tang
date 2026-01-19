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

# 编译器可执行文件
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
