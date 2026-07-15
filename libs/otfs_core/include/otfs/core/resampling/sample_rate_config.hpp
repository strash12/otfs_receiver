#pragma once

#include <cmath>
#include <cstddef>
#include <limits>

namespace otfs::core {

struct sample_rate_config {
    double input_sample_rate_hz{1.92e6};
    double processing_sample_rate_hz{0.96e6};

    [[nodiscard]] constexpr bool valid() const noexcept {
        if (input_sample_rate_hz <= 0.0 ||
            processing_sample_rate_hz <= 0.0 ||
            input_sample_rate_hz <
                processing_sample_rate_hz) {
            return false;
        }

        const double ratio =
            input_sample_rate_hz /
            processing_sample_rate_hz;

        const double rounded =
            static_cast<double>(
                static_cast<std::size_t>(
                    ratio + 0.5));

        return std::abs(ratio - rounded) <= 1e-9;
    }

    [[nodiscard]] constexpr std::size_t
    decimation_factor() const noexcept
    {
        return valid()
            ? static_cast<std::size_t>(
                  input_sample_rate_hz /
                      processing_sample_rate_hz +
                  0.5)
            : 0U;
    }

    [[nodiscard]] constexpr std::size_t
    to_input_samples(
        std::size_t processing_samples) const noexcept
    {
        return processing_samples *
               decimation_factor();
    }

    [[nodiscard]] constexpr bool input_length_valid(
        std::size_t input_samples) const noexcept
    {
        const auto factor = decimation_factor();

        return factor > 0U &&
               input_samples % factor == 0U;
    }

    [[nodiscard]] constexpr std::size_t
    to_processing_samples(
        std::size_t input_samples) const noexcept
    {
        const auto factor = decimation_factor();

        return factor > 0U
            ? input_samples / factor
            : 0U;
    }
};

} // namespace otfs::core
