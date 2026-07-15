find_package(PkgConfig REQUIRED)
pkg_check_modules(FFTW3F REQUIRED IMPORTED_TARGET fftw3f)

add_library(otfs_core
    src/version.cpp
    src/fft/fft_plan.cpp
    src/waveform/otfs_waveform.cpp
    src/pilot/pilot_masks.cpp
    src/frontend/dc_remover.cpp
    src/frontend/power_meter.cpp
    src/frontend/rms_agc.cpp
    src/frontend/radio_frontend.cpp
    src/synchronization/chirp_synchronizer.cpp
    src/cfo/cfo_estimator.cpp
    src/cfo/cfo_corrector.cpp
)

add_library(otfs::core ALIAS otfs_core)

target_include_directories(otfs_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(otfs_core PUBLIC cxx_std_20)

target_link_libraries(otfs_core
    PRIVATE
        PkgConfig::FFTW3F
)

otfs_set_project_warnings(otfs_core)
otfs_enable_optimizations(otfs_core)
otfs_enable_sanitizers(otfs_core)
