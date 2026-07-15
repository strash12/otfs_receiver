#pragma once

#include "otfs/core/types.hpp"

#include <cstddef>
#include <span>

namespace otfs::core {

struct repeated_block_cfo_config {
    double sample_rate_hz{1.92e6};
    std::size_t block_length{64};
    std::size_t repetitions{8};
    double minimum_correlation{1e-8};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return sample_rate_hz > 0.0 &&
               block_length > 0U &&
               repetitions >= 2U &&
               minimum_correlation >= 0.0;
    }
};

struct cfo_estimate {
    bool valid{};
    double frequency_hz{};
    double normalized_frequency{};
    double confidence{};
    double correlation_magnitude{};
};

class repeated_block_cfo_estimator final {
public:
    explicit repeated_block_cfo_estimator(
        const repeated_block_cfo_config& config);

    [[nodiscard]] cfo_estimate estimate(
        std::span<const sample_t> samples) const noexcept;

    [[nodiscard]] const repeated_block_cfo_config&
    config() const noexcept;

private:
    repeated_block_cfo_config config_{};
};

} // namespace otfs::core
