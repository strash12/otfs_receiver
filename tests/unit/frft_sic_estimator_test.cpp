#include "otfs/core/channel_estimation/fractional_reference.hpp"
#include "otfs/core/channel_estimation/frft_sic_estimator.hpp"
#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/model/config.hpp"
#include "otfs/core/types.hpp"
#include "otfs/core/waveform/guard_interval.hpp"
#include "otfs/core/waveform/otfs_waveform.hpp"

#include <cmath>
#include <cstdlib>

int main()
{
    constexpr otfs::core::receiver_config cfg{};

    otfs::core::otfs_waveform waveform{
        cfg.waveform,
        otfs::core::guard_interval::zero_padding};

    otfs::core::aligned_buffer<
        otfs::core::sample_t> pilot_dd{
            cfg.waveform.dd_cells()};

    pilot_dd.fill({});

    pilot_dd[
        otfs::core::column_major_index(
            cfg.pilot.delay_index,
            cfg.pilot.doppler_index,
            cfg.waveform.delay_bins)] = {
                cfg.pilot.amplitude,
                0.0F
            };

    otfs::core::aligned_buffer<
        otfs::core::sample_t> pilot_time{
            waveform.time_domain_samples()};

    if (!waveform.modulate(
            pilot_dd.span(),
            pilot_time.span())) {
        return EXIT_FAILURE;
    }

    otfs::core::fractional_reference_generator generator{
        pilot_time.span(),
        cfg,
        otfs::core::guard_interval::zero_padding};

    constexpr otfs::core::channel_path truth[] = {
        {{0.82F, -0.20F}, 2.18, -0.42},
        {{0.44F,  0.31F}, 6.73,  1.28},
        {{0.21F, -0.10F}, 10.35, -1.62}
    };

    otfs::core::aligned_buffer<
        otfs::core::sample_t> observation{
            generator.observation_size()};

    otfs::core::aligned_buffer<
        otfs::core::sample_t> reference{
            generator.observation_size()};

    observation.fill({});

    for (const auto& path : truth) {
        if (!generator.make_observation_reference(
                path.delay_samples,
                path.doppler_bins,
                reference.span())) {
            return EXIT_FAILURE;
        }

        for (std::size_t index = 0;
             index < observation.size();
             ++index) {
            observation[index] +=
                path.gain * reference[index];
        }
    }

    constexpr otfs::core::channel_path seeds[] = {
        {{}, 2.0,  0.0},
        {{}, 7.0,  1.0},
        {{}, 10.0, -2.0}
    };

    const otfs::core::frft_sic_config sic_config{
        .maximum_paths = 6U,
        .refinement_passes = 2U,
        .minimum_correlation = 0.10,
        .residual_ratio_stop = 1e-8,
        .minimum_relative_power = 1e-4,
        .ls_regularization = 1e-8,
        .local_search = {
            .fine_points = 21U,
            .delay_radius = 0.70,
            .doppler_radius = 0.80,
            .minimum_curvature = 1e-12
        }
    };

    otfs::core::frft_sic_estimator estimator{
        pilot_time.span(),
        cfg,
        otfs::core::guard_interval::zero_padding,
        sic_config};

    otfs::core::aligned_buffer<
        otfs::core::channel_path> estimated{
            estimator.maximum_paths()};

    const auto result = estimator.estimate(
        observation.span(),
        seeds,
        estimated.span());

    if (!result.valid ||
        result.detected_paths != 3U ||
        result.refinement_passes != 2U ||
        result.residual_ratio > 1e-2) {
        return EXIT_FAILURE;
    }

    for (const auto& expected : truth) {
        bool found = false;

        for (std::size_t index = 0;
             index < result.detected_paths;
             ++index) {
            const auto& actual = estimated[index];

            if (std::abs(
                    actual.delay_samples -
                    expected.delay_samples) < 0.12 &&
                std::abs(
                    actual.doppler_bins -
                    expected.doppler_bins) < 0.15 &&
                std::abs(
                    actual.gain -
                    expected.gain) < 0.15F) {
                found = true;
                break;
            }
        }

        if (!found) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
