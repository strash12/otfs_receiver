#pragma once

#include "otfs/core/model/channel.hpp"
#include "otfs/core/model/metrics.hpp"
#include "otfs/core/model/status.hpp"
#include "otfs/core/types.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace otfs::core {

struct frame_buffers {
    std::span<const sample_t> radio_capture{};
    std::span<sample_t> synchronized_frame{};
    std::span<sample_t> baseband_frame{};
    std::span<sample_t> delay_doppler_grid{};
    std::span<sample_t> equalized_time_samples{};
    std::span<sample_t> equalized_symbols{};

    [[nodiscard]] constexpr bool empty() const noexcept {
        return radio_capture.empty() &&
               synchronized_frame.empty() &&
               baseband_frame.empty() &&
               delay_doppler_grid.empty() &&
               equalized_time_samples.empty() &&
               equalized_symbols.empty();
    }
};

struct frame_context {
    std::uint64_t sequence{};
    receiver_mode mode{receiver_mode::full_recovery};
    processing_status status{processing_status::empty};

    std::size_t capture_offset{};
    std::size_t frame_start{};
    std::size_t frame_length{};

    frame_buffers buffers{};
    channel_estimate_view channel{};
    frame_metrics metrics{};

    bool header_valid{};
    bool payload_valid{};

    void reset_metadata(std::uint64_t new_sequence) noexcept {
        sequence = new_sequence;
        mode = receiver_mode::full_recovery;
        status = processing_status::empty;
        capture_offset = 0;
        frame_start = 0;
        frame_length = 0;
        channel = {};
        metrics = {};
        metrics.frame_sequence = new_sequence;
        header_valid = false;
        payload_valid = false;
    }
};

} // namespace otfs::core
