#pragma once

#include "otfs/core/channel_estimation/fractional_reference.hpp"
#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/types.hpp"

#include <cstddef>
#include <span>

namespace otfs::core {

struct frft_local_search_config {
    std::size_t fine_points{15};
    double delay_radius{0.30};
    double doppler_radius{0.30};
    double minimum_curvature{1e-12};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return fine_points >= 3U &&
               (fine_points % 2U) == 1U &&
               delay_radius > 0.0 &&
               doppler_radius > 0.0 &&
               minimum_curvature > 0.0;
    }
};

struct frft_local_search_result {
    bool valid{};
    double delay_samples{};
    double doppler_bins{};
    sample_t gain{};
    double peak_metric{};
    double normalized_correlation{};
    std::size_t evaluated_candidates{};
};

class frft_local_search final {
public:
    frft_local_search(
        std::span<const sample_t> pilot_time,
        const receiver_config& receiver_config,
        guard_interval guard_mode,
        const frft_local_search_config& search_config);

    [[nodiscard]] frft_local_search_result search(
        std::span<const sample_t> residual_observation,
        double center_delay,
        double center_doppler) noexcept;

    [[nodiscard]] std::size_t observation_size() const noexcept;

private:
    [[nodiscard]] double grid_value(
        double center,
        double radius,
        std::size_t index) const noexcept;

    [[nodiscard]] double parabolic_coordinate(
        std::span<const double> metric,
        std::size_t row,
        std::size_t column,
        bool delay_axis,
        double center_delay,
        double center_doppler) const noexcept;

    [[nodiscard]] sample_t estimate_gain(
        std::span<const sample_t> reference,
        std::span<const sample_t> residual) const noexcept;

    fractional_reference_generator reference_generator_;
    frft_local_search_config config_{};

    aligned_buffer<sample_t> candidate_reference_;
    aligned_buffer<sample_t> best_reference_;
    aligned_buffer<double> metric_;
};

} // namespace otfs::core
