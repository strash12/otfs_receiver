#include "otfs/core/pilot/pilot_observation.hpp"

#include "otfs/core/types.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace otfs::core {

pilot_observation_extractor::pilot_observation_extractor(
    const receiver_config& config)
    : config_(config),
      masks_(config),
      observation_indices_(
          masks_.observation_cells()),
      noise_indices_(
          masks_.noise_cells())
{
    if (!config_.valid()) {
        throw std::invalid_argument(
            "Invalid receiver configuration");
    }

    std::size_t observation_position = 0U;
    std::size_t noise_position = 0U;

    for (std::size_t index = 0;
         index < config_.waveform.dd_cells();
         ++index) {
        if (masks_.is_observation(index)) {
            observation_indices_[observation_position++] =
                index;
        }

        if (masks_.is_noise(index)) {
            noise_indices_[noise_position++] =
                index;
        }
    }
}

bool pilot_observation_extractor::process(
    std::span<const sample_t> delay_doppler_grid,
    std::span<sample_t> normalized_observation,
    pilot_observation_metrics& metrics) const noexcept
{
    metrics = {};

    if (delay_doppler_grid.size() !=
            config_.waveform.dd_cells() ||
        normalized_observation.size() !=
            observation_indices_.size()) {
        return false;
    }

    const float pilot_amplitude =
        config_.pilot.amplitude;

    if (pilot_amplitude <= 0.0F) {
        return false;
    }

    double observation_power = 0.0;
    double peak_power = -1.0;
    std::size_t peak_position = 0U;

    for (std::size_t position = 0;
         position < observation_indices_.size();
         ++position) {
        const std::size_t grid_index =
            observation_indices_[position];

        const sample_t normalized =
            delay_doppler_grid[grid_index] /
            pilot_amplitude;

        normalized_observation[position] =
            normalized;

        const double power =
            static_cast<double>(
                std::norm(normalized));

        observation_power += power;

        if (power > peak_power) {
            peak_power = power;
            peak_position = position;
        }
    }

    double noise_power = 0.0;

    for (const std::size_t grid_index :
         noise_indices_) {
        const sample_t normalized =
            delay_doppler_grid[grid_index] /
            pilot_amplitude;

        noise_power += static_cast<double>(
            std::norm(normalized));
    }

    const double noise_variance =
        noise_indices_.empty()
            ? 0.0
            : noise_power /
              static_cast<double>(
                  noise_indices_.size());

    const std::size_t peak_linear_index =
        observation_indices_[peak_position];

    metrics.noise_variance =
        noise_variance;

    metrics.observation_power =
        observation_indices_.empty()
            ? 0.0
            : observation_power /
              static_cast<double>(
                  observation_indices_.size());

    metrics.peak_power =
        std::max(peak_power, 0.0);

    metrics.peak_to_noise_db =
        10.0 *
        std::log10(
            (metrics.peak_power +
             std::numeric_limits<double>::epsilon()) /
            (metrics.noise_variance +
             std::numeric_limits<double>::epsilon()));

    metrics.peak_linear_index =
        peak_linear_index;

    metrics.peak_delay_index =
        peak_linear_index %
        config_.waveform.delay_bins;

    metrics.peak_doppler_index =
        peak_linear_index /
        config_.waveform.delay_bins;

    metrics.valid =
        std::isfinite(metrics.noise_variance) &&
        std::isfinite(metrics.observation_power) &&
        std::isfinite(metrics.peak_power) &&
        std::isfinite(metrics.peak_to_noise_db);

    return metrics.valid;
}

std::size_t
pilot_observation_extractor::observation_size() const noexcept
{
    return observation_indices_.size();
}

std::span<const std::size_t>
pilot_observation_extractor::observation_indices() const noexcept
{
    return observation_indices_.span();
}

} // namespace otfs::core
