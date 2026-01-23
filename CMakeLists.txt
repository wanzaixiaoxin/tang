cmake_minimum_required(VERSION 3.16)

project(tang_cpp20_coroutine_framework VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

# 根据编译器类型设置不同的警告选项
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "使用G++编译器: ${CMAKE_CXX_COMPILER_VERSION}")
    # G++的警告选项
    add_compile_options(-Wall -Wextra -Wpedantic -fcoroutines)
    add_link_options(-fcoroutines)
    # 根据编译模式设置不同的编译选项
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g -O2 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os -DNDEBUG")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(STATUS "检测到MSVC编译器: ${CMAKE_CXX_COMPILER_VERSION}")
    # MSVC的警告选项，注意使用/await:strict启用标准C++20协程支持
    add_compile_options(/W4 /await:strict)
    # 根据编译模式设置不同的编译选项
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /Od /DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Zi /O2 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /Os /DNDEBUG")
else()
    message(WARNING "检测到非G++编译器: ${CMAKE_CXX_COMPILER_ID}")
    message(WARNING "建议使用G++编译器以获得最佳C++20协程支持")
    # 默认警告选项
    add_compile_options(-Wall -fcoroutines)
    add_link_options(-fcoroutines)
endif()

include_directories(include)

set(COMMON_SOURCES
    src/runtime.cpp
    src/event_poller.cpp
    src/event_loop.cpp
)

if(WIN32)
    set(SOURCE_FILES 
        ${COMMON_SOURCES}
        src/event_poller_win.cpp
    )
elseif(UNIX)
    set(SOURCE_FILES
        ${COMMON_SOURCES}
        src/event_poller_linux.cpp
    )
endif()

add_library(tang STATIC ${SOURCE_FILES})
target_link_libraries(tang PUBLIC Threads::Threads)

install(TARGETS tang DESTINATION lib)
install(DIRECTORY include/tang DESTINATION include)

add_subdirectory(examples)

enable_testing()
add_subdirectory(tests)
