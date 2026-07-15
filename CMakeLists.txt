cmake_minimum_required(VERSION 3.22)

project(
    otfs_receiver
    VERSION 0.1.0
    DESCRIPTION "Modular C++20 OTFS research receiver"
    LANGUAGES CXX
)

option(OTFS_BUILD_TESTS "Build tests" ON)
option(OTFS_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(OTFS_ENABLE_NATIVE "Enable native CPU optimizations" ON)
option(OTFS_ENABLE_SANITIZERS "Enable ASan and UBSan" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerOptions)

add_subdirectory(libs/otfs_core)
add_subdirectory(libs/otfs_io)
add_subdirectory(apps/otfs_rx_offline)

if(OTFS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
