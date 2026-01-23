# 示例代码配置

# 示例列表
set(EXAMPLES
    hello_world
    channel_example
    select_example
    defer_example
)

# Linux-specific examples
if(UNIX)
    list(APPEND EXAMPLES async_io_example)
endif()

# 为每个示例创建可执行文件
foreach(example ${EXAMPLES})
    add_executable(${example} ${example}.cpp)
    target_link_libraries(${example} tang)
endforeach()
