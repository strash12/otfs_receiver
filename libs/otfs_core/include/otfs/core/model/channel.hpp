#pragma once

#include "otfs/core/types.hpp"

#include <cstddef>
#include <span>

namespace otfs::core {

struct channel_path {
    sample_t gain{};
    double delay_samples{};
    double doppler_bins{};
};

struct channel_estimate_view {
    std::span<const channel_path> paths{};
    double noise_variance{};
    double quality{};
    bool valid{};

    [[nodiscard]] constexpr std::size_t path_count() const noexcept {
        return paths.size();
    }
};

} // namespace otfs::core
