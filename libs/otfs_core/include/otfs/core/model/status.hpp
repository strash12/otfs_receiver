#pragma once

#include <cstdint>

namespace otfs::core {

enum class processing_status : std::uint8_t {
    empty,
    synchronized,
    frame_extracted,
    demodulated,
    channel_estimated,
    equalized,
    decoded,
    failed
};

enum class receiver_mode : std::uint8_t {
    track,
    key_recovery,
    full_recovery
};

} // namespace otfs::core
