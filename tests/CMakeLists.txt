add_executable(otfs_core_smoke_test
    unit/core_smoke_test.cpp
)

target_link_libraries(otfs_core_smoke_test PRIVATE otfs::core)

otfs_set_project_warnings(otfs_core_smoke_test)
otfs_enable_sanitizers(otfs_core_smoke_test)

add_test(NAME otfs_core_smoke_test COMMAND otfs_core_smoke_test)

add_executable(data_model_test
    unit/data_model_test.cpp
)

target_link_libraries(data_model_test PRIVATE otfs::core)

otfs_set_project_warnings(data_model_test)
otfs_enable_sanitizers(data_model_test)

add_test(NAME data_model_test COMMAND data_model_test)

add_executable(aligned_buffer_test
    unit/aligned_buffer_test.cpp
)

target_link_libraries(aligned_buffer_test PRIVATE otfs::core)

otfs_set_project_warnings(aligned_buffer_test)
otfs_enable_sanitizers(aligned_buffer_test)

add_test(NAME aligned_buffer_test COMMAND aligned_buffer_test)

add_executable(fft_plan_test
    unit/fft_plan_test.cpp
)

target_link_libraries(fft_plan_test PRIVATE otfs::core)

otfs_set_project_warnings(fft_plan_test)
otfs_enable_sanitizers(fft_plan_test)

add_test(NAME fft_plan_test COMMAND fft_plan_test)
