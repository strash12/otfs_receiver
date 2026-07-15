#pragma once

#include <cstdint>

namespace otfs::core {

enum class fft_direction : std::uint8_t {
    forward,
    inverse
};

} // namespace otfs::core
