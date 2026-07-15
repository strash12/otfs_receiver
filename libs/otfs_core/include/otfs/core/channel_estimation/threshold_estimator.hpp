#pragma once

#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/model/channel.hpp"
#include "otfs/core/model/config.hpp"
#include "otfs/core/pilot/pilot_observation.hpp"
#include "otfs/core/types.hpp"

#include <cstddef>
#include <span>

namespace otfs::core {

struct threshold_estimator_config {
    double threshold_sigma{5.0};
    double minimum_peak_to_noise_db{6.0};
    std::size_t maximum_paths{16};
    bool keep_strongest_if_empty{true};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return threshold_sigma > 0.0 &&
               minimum_peak_to_noise_db >= 0.0 &&
               maximum_paths > 0U;
    }
};

struct threshold_estimator_result {
    bool valid{};
    bool threshold_fallback_used{};
    std::size_t detected_paths{};
    double threshold_power{};
    double noise_variance{};
    double strongest_path_power{};
};

class threshold_channel_estimator final {
public:
    threshold_channel_estimator(
        const receiver_config& receiver_config,
        const threshold_estimator_config& estimator_config);

    threshold_channel_estimator(
        const threshold_channel_estimator&) = delete;
    threshold_channel_estimator& operator=(
        const threshold_channel_estimator&) = delete;
    threshold_channel_estimator(
        threshold_channel_estimator&&) noexcept = default;
    threshold_channel_estimator& operator=(
        threshold_channel_estimator&&) noexcept = default;

    [[nodiscard]] threshold_estimator_result estimate(
        std::span<const sample_t> normalized_observation,
        const pilot_observation_metrics& observation_metrics,
        std::span<channel_path> output_paths) noexcept;

    [[nodiscard]] std::size_t maximum_paths() const noexcept;

private:
    struct candidate {
        std::size_t observation_position{};
        double power{};
    };

    receiver_config receiver_config_{};
    threshold_estimator_config estimator_config_{};
    pilot_observation_extractor observation_extractor_;
    aligned_buffer<candidate> candidates_;
};

} // namespace otfs::core
