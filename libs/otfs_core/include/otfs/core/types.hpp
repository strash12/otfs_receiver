#pragma once

#include <complex>
#include <cstddef>

namespace otfs::core {

using sample_t = std::complex<float>;
using accumulator_t = std::complex<double>;

struct dimensions {
    std::size_t delay_bins{};
    std::size_t doppler_bins{};
    std::size_t guard_samples{};

    [[nodiscard]] constexpr std::size_t dd_cells() const noexcept {
        return delay_bins * doppler_bins;
    }

    [[nodiscard]] constexpr std::size_t frame_samples() const noexcept {
        return (delay_bins + guard_samples) * doppler_bins;
    }
};

[[nodiscard]] constexpr std::size_t column_major_index(
    std::size_t delay,
    std::size_t doppler,
    std::size_t delay_bins) noexcept
{
    return delay + doppler * delay_bins;
}

} // namespace otfs::core
