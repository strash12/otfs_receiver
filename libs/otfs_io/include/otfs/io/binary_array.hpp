#pragma once

#include <filesystem>
#include <vector>

namespace otfs::io {

[[nodiscard]] bool read_float64_le(
    const std::filesystem::path& path,
    std::vector<double>& output);

} // namespace otfs::io
