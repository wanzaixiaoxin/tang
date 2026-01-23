# 单元测试配置

# 创建测试框架库
add_library(test_framework STATIC test_framework.cpp)
target_link_libraries(test_framework tang)

# 为测试框架库添加G++特定选项
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(test_framework PRIVATE -fcoroutines)
    target_link_options(test_framework PRIVATE -fcoroutines)
endif()

# 测试列表
set(TESTS
    task_test
    channel_test
    select_test
    runtime_test
    integration_test
    simple_test
)

# 为每个测试创建可执行文件（独立版本，不依赖Google Test）
foreach(test ${TESTS})
    add_executable(${test} ${test}.cpp)
    target_link_libraries(${test} test_framework tang)
    
    # 设置输出目录
    set_target_properties(${test} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )
    
    # 为测试目标添加G++特定选项
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${test} PRIVATE -fcoroutines)
        target_link_options(${test} PRIVATE -fcoroutines)
    endif()
    
    message(STATUS "Configured test: ${test}")
endforeach()
