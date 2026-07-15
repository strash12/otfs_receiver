add_executable(otfs_core_smoke_test
    unit/core_smoke_test.cpp
)

target_link_libraries(otfs_core_smoke_test PRIVATE otfs::core)

otfs_set_project_warnings(otfs_core_smoke_test)
otfs_enable_sanitizers(otfs_core_smoke_test)

add_test(NAME otfs_core_smoke_test COMMAND otfs_core_smoke_test)
