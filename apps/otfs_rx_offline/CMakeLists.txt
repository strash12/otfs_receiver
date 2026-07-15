add_executable(otfs_rx_offline main.cpp)

target_link_libraries(otfs_rx_offline
    PRIVATE
        otfs::core
        otfs::io
)

otfs_set_project_warnings(otfs_rx_offline)
otfs_enable_optimizations(otfs_rx_offline)
otfs_enable_sanitizers(otfs_rx_offline)
