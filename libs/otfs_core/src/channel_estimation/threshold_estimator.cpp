#include "otfs/core/channel_estimation/threshold_estimator.hpp"

#include "otfs/core/types.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace otfs::core {

threshold_channel_estimator::threshold_channel_estimator(
    const receiver_config& receiver_config,
    const threshold_estimator_config& estimator_config)
    : receiver_config_(receiver_config),
      estimator_config_(estimator_config),
      observation_extractor_(receiver_config),
      candidates_(
          observation_extractor_.observation_size())
{
    if (!receiver_config_.valid()) {
        throw std::invalid_argument(
            "Invalid receiver configuration");
    }

    if (!estimator_config_.valid()) {
        throw std::invalid_argument(
            "Invalid threshold estimator configuration");
    }
}

threshold_estimator_result
threshold_channel_estimator::estimate(
    std::span<const sample_t> normalized_observation,
    const pilot_observation_metrics& observation_metrics,
    std::span<channel_path> output_paths) noexcept
{
    threshold_estimator_result result{};

    if (!observation_metrics.valid ||
        normalized_observation.size() !=
            observation_extractor_.observation_size() ||
        output_paths.size() <
            estimator_config_.maximum_paths) {
        return result;
    }

    const double threshold_power =
        estimator_config_.threshold_sigma *
        estimator_config_.threshold_sigma *
        observation_metrics.noise_variance;

    result.threshold_power = threshold_power;
    result.noise_variance =
        observation_metrics.noise_variance;
    result.strongest_path_power =
        observation_metrics.peak_power;

    std::size_t candidate_count = 0U;

    for (std::size_t position = 0;
         position < normalized_observation.size();
         ++position) {
        const double power =
            static_cast<double>(
                std::norm(
                    normalized_observation[position]));

        if (power >= threshold_power) {
            candidates_[candidate_count] = {
                .observation_position = position,
                .power = power
            };
            ++candidate_count;
        }
    }

    if (candidate_count == 0U &&
        estimator_config_.keep_strongest_if_empty &&
        observation_metrics.peak_to_noise_db >=
            estimator_config_.minimum_peak_to_noise_db) {
        const auto indices =
            observation_extractor_.observation_indices();

        std::size_t strongest_position = 0U;

        for (std::size_t position = 0;
             position < indices.size();
             ++position) {
            if (indices[position] ==
                observation_metrics.peak_linear_index) {
                strongest_position = position;
                break;
            }
        }

        candidates_[0] = {
            .observation_position = strongest_position,
            .power = observation_metrics.peak_power
        };

        candidate_count = 1U;
        result.threshold_fallback_used = true;
    }

    if (candidate_count == 0U) {
        result.valid = true;
        return result;
    }

    std::sort(
        candidates_.begin(),
        candidates_.begin() +
            static_cast<std::ptrdiff_t>(
                candidate_count),
        [](const candidate& lhs,
           const candidate& rhs) {
            return lhs.power > rhs.power;
        });

    const std::size_t selected =
        std::min(
            candidate_count,
            estimator_config_.maximum_paths);

    const auto observation_indices =
        observation_extractor_.observation_indices();

    for (std::size_t path_index = 0;
         path_index < selected;
         ++path_index) {
        const std::size_t observation_position =
            candidates_[path_index]
                .observation_position;

        const std::size_t linear_index =
            observation_indices[
                observation_position];

        const std::size_t delay_index =
            linear_index %
            receiver_config_.waveform.delay_bins;

        const std::size_t doppler_index =
            linear_index /
            receiver_config_.waveform.delay_bins;

        const double relative_delay =
            static_cast<double>(delay_index) -
            static_cast<double>(
                receiver_config_.pilot.delay_index);

        double relative_doppler =
            static_cast<double>(doppler_index) -
            static_cast<double>(
                receiver_config_.pilot.doppler_index);

        const double half_doppler_grid =
            0.5 *
            static_cast<double>(
                receiver_config_.waveform.doppler_bins);

        if (relative_doppler > half_doppler_grid) {
            relative_doppler -=
                static_cast<double>(
                    receiver_config_.waveform.doppler_bins);
        }
        else if (relative_doppler < -half_doppler_grid) {
            relative_doppler +=
                static_cast<double>(
                    receiver_config_.waveform.doppler_bins);
        }

        output_paths[path_index] = {
            .gain =
                normalized_observation[
                    observation_position],
            .delay_samples = relative_delay,
            .doppler_bins = relative_doppler
        };
    }

    result.detected_paths = selected;
    result.valid = true;

    return result;
}

std::size_t
threshold_channel_estimator::maximum_paths() const noexcept
{
    return estimator_config_.maximum_paths;
}

} // namespace otfs::core
