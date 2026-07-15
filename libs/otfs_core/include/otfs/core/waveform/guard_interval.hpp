#pragma once

#include <cstdint>
#include <string_view>

namespace otfs::core {

enum class guard_interval : std::uint8_t {
    cyclic_prefix,
    reduced_cyclic_prefix,
    zero_padding,
    reduced_zero_padding,
    none
};

[[nodiscard]] constexpr std::string_view to_string(
    guard_interval mode) noexcept
{
    switch (mode) {
    case guard_interval::cyclic_prefix:
        return "CP";
    case guard_interval::reduced_cyclic_prefix:
        return "RCP";
    case guard_interval::zero_padding:
        return "ZP";
    case guard_interval::reduced_zero_padding:
        return "RZP";
    case guard_interval::none:
        return "NONE";
    }
    return "UNKNOWN";
}

} // namespace otfs::core
