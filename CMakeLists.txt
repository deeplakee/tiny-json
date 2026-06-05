cmake_minimum_required(VERSION 3.29)
project(tiny_json)

set(CMAKE_CXX_STANDARD 20)

# 头文件库目标（供外部项目通过 add_subdirectory 或 find_package 使用）
add_library(tiny_json INTERFACE)
target_include_directories(tiny_json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(tiny_json INTERFACE cxx_std_20)

# 测试可执行文件
add_executable(universal_test universal_test.cpp)
target_link_libraries(universal_test PRIVATE tiny_json)

add_executable(utf_8_test utf_8_test.cpp)
target_link_libraries(utf_8_test PRIVATE tiny_json)

# 编译器警告
target_compile_options(universal_test PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(utf_8_test PRIVATE -Wall -Wextra -Wpedantic)

# CTest 集成
enable_testing()
add_test(NAME universal_test COMMAND universal_test)
add_test(NAME utf_8_test COMMAND utf_8_test)
