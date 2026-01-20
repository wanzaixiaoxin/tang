# 单元测试配置

# 测试列表
set(TESTS
    task_test
    channel_test
    select_test
)

# 如果找到Google Test，则启用测试
find_package(GTest QUIET)

if(GTest_FOUND)
    message(STATUS "Found Google Test: ${GTEST_VERSION}")
    
    # 为每个测试创建可执行文件
    foreach(test ${TESTS})
        add_executable(${test} ${test}.cpp)
        target_link_libraries(${test} tang GTest::GTest GTest::Main)
        add_test(NAME ${test} COMMAND ${test})
    endforeach()
else()
    message(STATUS "Google Test not found, tests will be skipped")
    message(STATUS "To enable tests, install Google Test or use -DGTEST_ROOT to specify its location")
endif()
