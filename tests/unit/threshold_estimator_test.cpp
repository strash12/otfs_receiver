#include "otfs/core/channel_estimation/threshold_estimator.hpp"
#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/model/channel.hpp"
#include "otfs/core/model/config.hpp"
#include "otfs/core/pilot/pilot_observation.hpp"
#include "otfs/core/types.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

int main()
{
    constexpr otfs::core::receiver_config receiver_config{};

    otfs::core::pilot_observation_extractor extractor{
        receiver_config};

    otfs::core::aligned_buffer<
        otfs::core::sample_t> grid{
            receiver_config.waveform.dd_cells()};

    otfs::core::aligned_buffer<
        otfs::core::sample_t> observation{
            extractor.observation_size()};

    grid.fill({});

    struct synthetic_path {
        std::size_t delay_offset;
        int doppler_offset;
        otfs::core::sample_t gain;
    };

    constexpr synthetic_path paths[] = {
        {2U, -1, {0.90F, 0.10F}},
        {7U,  2, {0.45F, -0.30F}},
        {11U, 0, {0.20F, 0.05F}}
    };

    for (const auto& path : paths) {
        const auto delay =
            receiver_config.pilot.delay_index +
            path.delay_offset;

        const auto doppler =
            static_cast<std::size_t>(
                static_cast<int>(
                    receiver_config.pilot.doppler_index) +
                path.doppler_offset);

        const auto index =
            otfs::core::column_major_index(
                delay,
                doppler,
                receiver_config.waveform.delay_bins);

        grid[index] =
            path.gain *
            receiver_config.pilot.amplitude;
    }

    // Deterministic low-level noise in all other cells.
    for (std::size_t index = 0;
         index < grid.size();
         ++index) {
        if (grid[index] ==
            otfs::core::sample_t{}) {
            const float value =
                static_cast<float>(
                    1U + (index % 5U)) *
                2e-4F;

            grid[index] = {
                value,
                -0.25F * value
            };
        }
    }

    otfs::core::pilot_observation_metrics observation_metrics{};

    if (!extractor.process(
            grid.span(),
            observation.span(),
            observation_metrics)) {
        return EXIT_FAILURE;
    }

    const otfs::core::threshold_estimator_config
        estimator_config{
            .threshold_sigma = 6.0,
            .minimum_peak_to_noise_db = 6.0,
            .maximum_paths = 8U,
            .keep_strongest_if_empty = true
        };

    otfs::core::threshold_channel_estimator estimator{
        receiver_config,
        estimator_config};

    otfs::core::aligned_buffer<
        otfs::core::channel_path> estimated_paths{
            estimator.maximum_paths()};

    const auto result =
        estimator.estimate(
            observation.span(),
            observation_metrics,
            estimated_paths.span());

    if (!result.valid ||
        result.detected_paths != 3U ||
        result.threshold_fallback_used ||
        result.threshold_power <= 0.0 ||
        result.noise_variance <= 0.0) {
        return EXIT_FAILURE;
    }

    const auto approximately_equal =
        [](double lhs,
           double rhs,
           double tolerance) {
            return std::abs(lhs - rhs) <= tolerance;
        };

    bool found_first = false;
    bool found_second = false;
    bool found_third = false;

    for (std::size_t index = 0;
         index < result.detected_paths;
         ++index) {
        const auto& path =
            estimated_paths[index];

        if (approximately_equal(
                path.delay_samples,
                2.0,
                1e-12) &&
            approximately_equal(
                path.doppler_bins,
                -1.0,
                1e-12) &&
            std::abs(
                path.gain -
                paths[0].gain) < 1e-6F) {
            found_first = true;
        }

        if (approximately_equal(
                path.delay_samples,
                7.0,
                1e-12) &&
            approximately_equal(
                path.doppler_bins,
                2.0,
                1e-12) &&
            std::abs(
                path.gain -
                paths[1].gain) < 1e-6F) {
            found_second = true;
        }

        if (approximately_equal(
                path.delay_samples,
                11.0,
                1e-12) &&
            approximately_equal(
                path.doppler_bins,
                0.0,
                1e-12) &&
            std::abs(
                path.gain -
                paths[2].gain) < 1e-6F) {
            found_third = true;
        }
    }

    if (!found_first ||
        !found_second ||
        !found_third) {
        return EXIT_FAILURE;
    }

    const otfs::core::threshold_estimator_config
        fallback_config{
            .threshold_sigma = 1e6,
            .minimum_peak_to_noise_db = 6.0,
            .maximum_paths = 4U,
            .keep_strongest_if_empty = true
        };

    otfs::core::threshold_channel_estimator
        fallback_estimator{
            receiver_config,
            fallback_config};

    otfs::core::aligned_buffer<
        otfs::core::channel_path> fallback_paths{
            fallback_estimator.maximum_paths()};

    const auto fallback_result =
        fallback_estimator.estimate(
            observation.span(),
            observation_metrics,
            fallback_paths.span());

    if (!fallback_result.valid ||
        !fallback_result.threshold_fallback_used ||
        fallback_result.detected_paths != 1U) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
