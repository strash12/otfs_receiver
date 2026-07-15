add_library(otfs_core
    src/version.cpp
)

add_library(otfs::core ALIAS otfs_core)

target_include_directories(otfs_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(otfs_core PUBLIC cxx_std_20)

otfs_set_project_warnings(otfs_core)
otfs_enable_optimizations(otfs_core)
otfs_enable_sanitizers(otfs_core)
