cmake_minimum_required(VERSION 3.29)
project(tiny_json)

set(CMAKE_CXX_STANDARD 20)

# 头文件库目标（供外部项目通过 add_subdirectory 或 find_package 使用）
add_library(tiny_json INTERFACE)
target_include_directories(tiny_json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(tiny_json INTERFACE cxx_std_20)

# 测试可执行文件
set(TEST_SOURCES
    test_parse.cpp
    test_serialize.cpp
    test_accessor.cpp
    test_json_pointer.cpp
    test_merge_patch.cpp
)

enable_testing()

foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE tiny_json)
    target_compile_options(${test_name} PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
