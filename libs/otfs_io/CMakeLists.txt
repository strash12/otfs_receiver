add_library(otfs_io
    src/iq_format.cpp
    src/reference_data.cpp
    src/binary_array.cpp
)

add_library(otfs::io ALIAS otfs_io)

target_include_directories(otfs_io
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(otfs_io PUBLIC otfs::core)
target_compile_features(otfs_io PUBLIC cxx_std_20)

otfs_set_project_warnings(otfs_io)
otfs_enable_optimizations(otfs_io)
otfs_enable_sanitizers(otfs_io)
