#pragma once

#include <cstddef>
#include <cstdint>

namespace otfs::core {

struct synchronization_metrics {
    bool found{};
    std::size_t chirp_start{};
    float matched_filter_peak_db{};
    float normalized_correlation{};
};

struct frequency_metrics {
    double coarse_cfo_hz{};
    double residual_cfo_hz{};
    bool coarse_valid{};
    bool residual_valid{};
};

struct equalizer_metrics {
    std::size_t iterations{};
    double relative_residual{};
    bool converged{};
    bool breakdown{};
};

struct link_metrics {
    double snr_db{};
    double evm_percent{};
    bool header_crc_ok{};
    bool payload_crc_ok{};
    std::size_t corrected_fec_words{};
};

struct timing_metrics {
    double synchronization_us{};
    double cfo_us{};
    double demodulation_us{};
    double channel_estimation_us{};
    double equalization_us{};
    double decoding_us{};
    double total_us{};
};

struct frame_metrics {
    std::uint64_t frame_sequence{};
    synchronization_metrics synchronization{};
    frequency_metrics frequency{};
    equalizer_metrics equalizer{};
    link_metrics link{};
    timing_metrics timing{};
};

} // namespace otfs::core
