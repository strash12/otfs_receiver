#pragma once

#include "otfs/core/types.hpp"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace otfs::io {

[[nodiscard]] bool read_complex_float32_le(
    const std::filesystem::path& path,
    std::vector<otfs::core::sample_t>& output);

[[nodiscard]] bool read_uint8(
    const std::filesystem::path& path,
    std::vector<std::uint8_t>& output);

} // namespace otfs::io
