cmake_minimum_required(VERSION 3.29)
project(tiny_json)

set(CMAKE_CXX_STANDARD 20)

add_executable(universal_test universal_test.cpp
        json.h
        test.h)

add_executable(utf_8_test utf_8_test.cpp
        json.h
        test.h)