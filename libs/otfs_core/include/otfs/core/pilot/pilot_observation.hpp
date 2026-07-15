#pragma once

#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/model/config.hpp"
#include "otfs/core/pilot/pilot_masks.hpp"
#include "otfs/core/types.hpp"

#include <cstddef>
#include <span>

namespace otfs::core {

struct pilot_observation_metrics {
    double noise_variance{};
    double observation_power{};
    double peak_power{};
    double peak_to_noise_db{};
    std::size_t peak_linear_index{};
    std::size_t peak_delay_index{};
    std::size_t peak_doppler_index{};
    bool valid{};
};

class pilot_observation_extractor final {
public:
    explicit pilot_observation_extractor(
        const receiver_config& config);

    pilot_observation_extractor(
        const pilot_observation_extractor&) = delete;
    pilot_observation_extractor& operator=(
        const pilot_observation_extractor&) = delete;
    pilot_observation_extractor(
        pilot_observation_extractor&&) noexcept = default;
    pilot_observation_extractor& operator=(
        pilot_observation_extractor&&) noexcept = default;

    [[nodiscard]] bool process(
        std::span<const sample_t> delay_doppler_grid,
        std::span<sample_t> normalized_observation,
        pilot_observation_metrics& metrics) const noexcept;

    [[nodiscard]] std::size_t observation_size() const noexcept;
    [[nodiscard]] std::span<const std::size_t>
    observation_indices() const noexcept;

private:
    receiver_config config_{};
    pilot_masks masks_;

    aligned_buffer<std::size_t> observation_indices_;
    aligned_buffer<std::size_t> noise_indices_;
};

} // namespace otfs::core
