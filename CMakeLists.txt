cmake_minimum_required(VERSION 3.16)

# 强制使用G++编译器
if(NOT DEFINED CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER "g++")
endif()

project(tang_cpp20_coroutine_framework VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

# 强制使用G++编译选项
add_compile_options(-Wall -Wextra -Wpedantic -fcoroutines)
add_link_options(-fcoroutines)

# 设置编译器特定标志
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "使用G++编译器: ${CMAKE_CXX_COMPILER_VERSION}")
    add_compile_options(-fcoroutines)
    add_link_options(-fcoroutines)
else()
    message(WARNING "检测到非G++编译器: ${CMAKE_CXX_COMPILER_ID}")
    message(WARNING "建议使用G++编译器以获得最佳C++20协程支持")
endif()

include_directories(include)

set(SOURCE_FILES
    src/runtime.cpp
)

add_library(tang STATIC ${SOURCE_FILES})
target_link_libraries(tang PUBLIC Threads::Threads)

install(TARGETS tang DESTINATION lib)
install(DIRECTORY include/tang DESTINATION include)

add_subdirectory(examples)

enable_testing()
add_subdirectory(tests)
