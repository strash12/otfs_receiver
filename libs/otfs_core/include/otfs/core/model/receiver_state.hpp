#pragma once

#include "otfs/core/model/status.hpp"

#include <cstddef>
#include <cstdint>

namespace otfs::core {

struct receiver_state {
    receiver_mode requested_mode{receiver_mode::full_recovery};

    std::uint64_t processed_frames{};
    std::uint64_t valid_frames{};
    std::uint64_t crc_failures{};
    std::uint64_t synchronization_failures{};
    std::uint64_t dropped_frames{};

    std::size_t consecutive_crc_failures{};
    std::size_t consecutive_fec_corrections{};
    std::size_t frames_since_full_recovery{};

    double tracked_cfo_hz{};
    bool tracked_cfo_valid{};
};

} // namespace otfs::core
