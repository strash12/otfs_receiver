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

add_executable(dsp_infrastructure_test
    unit/dsp_infrastructure_test.cpp
)

target_link_libraries(dsp_infrastructure_test PRIVATE otfs::core)

otfs_set_project_warnings(dsp_infrastructure_test)
otfs_enable_sanitizers(dsp_infrastructure_test)

add_test(NAME dsp_infrastructure_test COMMAND dsp_infrastructure_test)




add_executable(otfs_waveform_test
    unit/otfs_waveform_test.cpp
)

target_link_libraries(otfs_waveform_test PRIVATE otfs::core)

otfs_set_project_warnings(otfs_waveform_test)
otfs_enable_sanitizers(otfs_waveform_test)

add_test(NAME otfs_waveform_test COMMAND otfs_waveform_test)


add_executable(pilot_masks_test
    unit/pilot_masks_test.cpp
)

target_link_libraries(pilot_masks_test PRIVATE otfs::core)

otfs_set_project_warnings(pilot_masks_test)
otfs_enable_sanitizers(pilot_masks_test)

add_test(NAME pilot_masks_test COMMAND pilot_masks_test)


add_executable(matlab_reference_test
    regression/matlab_reference_test.cpp
)

target_link_libraries(matlab_reference_test
    PRIVATE
        otfs::core
        otfs::io
)

otfs_set_project_warnings(matlab_reference_test)
otfs_enable_sanitizers(matlab_reference_test)

add_test(
    NAME matlab_reference_test
    COMMAND matlab_reference_test
        ${CMAKE_CURRENT_SOURCE_DIR}/matlab_reference/generated
)

set_tests_properties(
    matlab_reference_test
    PROPERTIES
        SKIP_RETURN_CODE 77
)


add_executable(radio_frontend_test
    unit/radio_frontend_test.cpp
)

target_link_libraries(radio_frontend_test PRIVATE otfs::core)

otfs_set_project_warnings(radio_frontend_test)
otfs_enable_sanitizers(radio_frontend_test)

add_test(NAME radio_frontend_test COMMAND radio_frontend_test)


add_executable(chirp_synchronizer_test
    unit/chirp_synchronizer_test.cpp
)

target_link_libraries(chirp_synchronizer_test PRIVATE otfs::core)

otfs_set_project_warnings(chirp_synchronizer_test)
otfs_enable_sanitizers(chirp_synchronizer_test)

add_test(NAME chirp_synchronizer_test COMMAND chirp_synchronizer_test)


add_executable(cfo_processing_test
    unit/cfo_processing_test.cpp
)

target_link_libraries(cfo_processing_test PRIVATE otfs::core)

otfs_set_project_warnings(cfo_processing_test)
otfs_enable_sanitizers(cfo_processing_test)

add_test(NAME cfo_processing_test COMMAND cfo_processing_test)


add_executable(synchronization_pipeline_test
    unit/synchronization_pipeline_test.cpp
)

target_link_libraries(synchronization_pipeline_test PRIVATE otfs::core)

otfs_set_project_warnings(synchronization_pipeline_test)
otfs_enable_sanitizers(synchronization_pipeline_test)

add_test(
    NAME synchronization_pipeline_test
    COMMAND synchronization_pipeline_test
)


add_executable(frame_resampling_test
    unit/frame_resampling_test.cpp
)

target_link_libraries(frame_resampling_test PRIVATE otfs::core)

otfs_set_project_warnings(frame_resampling_test)
otfs_enable_sanitizers(frame_resampling_test)

add_test(NAME frame_resampling_test COMMAND frame_resampling_test)


add_executable(pilot_observation_test
    unit/pilot_observation_test.cpp
)

target_link_libraries(pilot_observation_test PRIVATE otfs::core)

otfs_set_project_warnings(pilot_observation_test)
otfs_enable_sanitizers(pilot_observation_test)

add_test(
    NAME pilot_observation_test
    COMMAND pilot_observation_test
)


add_executable(threshold_estimator_test
    unit/threshold_estimator_test.cpp
)

target_link_libraries(threshold_estimator_test PRIVATE otfs::core)

otfs_set_project_warnings(threshold_estimator_test)
otfs_enable_sanitizers(threshold_estimator_test)

add_test(
    NAME threshold_estimator_test
    COMMAND threshold_estimator_test
)


add_executable(fractional_peak_refiner_test
    unit/fractional_peak_refiner_test.cpp
)

target_link_libraries(fractional_peak_refiner_test PRIVATE otfs::core)

otfs_set_project_warnings(fractional_peak_refiner_test)
otfs_enable_sanitizers(fractional_peak_refiner_test)

add_test(
    NAME fractional_peak_refiner_test
    COMMAND fractional_peak_refiner_test
)


add_executable(frft_local_search_test
    unit/frft_local_search_test.cpp
)

target_link_libraries(frft_local_search_test PRIVATE otfs::core)

otfs_set_project_warnings(frft_local_search_test)
otfs_enable_sanitizers(frft_local_search_test)

add_test(
    NAME frft_local_search_test
    COMMAND frft_local_search_test
)


add_executable(fractional_reference_regression_test
    regression/fractional_reference_regression_test.cpp
)

target_link_libraries(fractional_reference_regression_test
    PRIVATE
        otfs::core
        otfs::io
)

otfs_set_project_warnings(
    fractional_reference_regression_test)

otfs_enable_sanitizers(
    fractional_reference_regression_test)

add_test(
    NAME fractional_reference_regression_test
    COMMAND fractional_reference_regression_test
        ${CMAKE_CURRENT_SOURCE_DIR}/matlab_reference/generated/fractional_reference
)

set_tests_properties(
    fractional_reference_regression_test
    PROPERTIES
        SKIP_RETURN_CODE 77
)
