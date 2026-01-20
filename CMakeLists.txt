cmake_minimum_required(VERSION 3.16)

project(tang_cpp20_coroutine_framework VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus /wd4819)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -fcoroutines)
    add_link_options(-fcoroutines)
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
