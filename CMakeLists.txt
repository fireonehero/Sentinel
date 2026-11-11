cmake_minimum_required(VERSION 3.20)
project(sentinel VERSION 0.1.0 DESCRIPTION "Network traffic analyzer with intrusion detection" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(sentinel
    src/main.cpp
    src/output.cpp
    src/capture.cpp
)

target_include_directories(sentinel PRIVATE include)
target_compile_options(sentinel PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(sentinel PRIVATE pcap)