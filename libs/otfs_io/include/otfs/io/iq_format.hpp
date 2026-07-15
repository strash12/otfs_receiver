#pragma once

#include <string_view>

namespace otfs::io {

enum class iq_format {
    complex_float32,
    interleaved_float32,
    interleaved_int16
};

[[nodiscard]] std::string_view to_string(iq_format format) noexcept;

} // namespace otfs::io
