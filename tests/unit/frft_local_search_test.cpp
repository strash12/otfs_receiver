#include "otfs/core/channel_estimation/fractional_reference.hpp"
#include "otfs/core/channel_estimation/frft_local_search.hpp"
#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/model/config.hpp"
#include "otfs/core/types.hpp"
#include "otfs/core/waveform/guard_interval.hpp"
#include "otfs/core/waveform/otfs_waveform.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>

int main()
{
    constexpr otfs::core::receiver_config config{};

    otfs::core::otfs_waveform waveform{
        config.waveform,
        otfs::core::guard_interval::zero_padding};

    otfs::core::aligned_buffer<
        otfs::core::sample_t> pilot_dd{
            config.waveform.dd_cells()};

    pilot_dd.fill({});

    const auto pilot_index =
        otfs::core::column_major_index(
            config.pilot.delay_index,
            config.pilot.doppler_index,
            config.waveform.delay_bins);

    pilot_dd[pilot_index] = {
        config.pilot.amplitude,
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
        config,
        otfs::core::guard_interval::zero_padding};

    constexpr double true_delay = 5.23;
    constexpr double true_doppler = -1.37;

    const otfs::core::sample_t true_gain{
        0.72F,
        -0.31F
    };

    otfs::core::aligned_buffer<
        otfs::core::sample_t> residual{
            generator.observation_size()};

    if (!generator.make_observation_reference(
            true_delay,
            true_doppler,
            residual.span())) {
        return EXIT_FAILURE;
    }

    for (auto& value : residual) {
        value *= true_gain;
    }

    const otfs::core::frft_local_search_config
        search_config{
            .fine_points = 21U,
            .delay_radius = 0.50,
            .doppler_radius = 0.50,
            .minimum_curvature = 1e-12
        };

    otfs::core::frft_local_search search{
        pilot_time.span(),
        config,
        otfs::core::guard_interval::zero_padding,
        search_config};

    const auto result =
        search.search(
            residual.span(),
            5.0,
            -1.0);

    std::cerr
        << "FRFT result:"
        << " valid=" << result.valid
        << " delay=" << result.delay_samples
        << " doppler=" << result.doppler_bins
        << " gain=" << result.gain
        << " metric=" << result.peak_metric
        << " rho=" << result.normalized_correlation
        << " candidates=" << result.evaluated_candidates
        << '\n';

    if (!result.valid ||
        result.evaluated_candidates != 441U ||
        result.normalized_correlation < 0.99) {
        return EXIT_FAILURE;
    }

    if (std::abs(
            result.delay_samples -
            true_delay) > 0.05 ||
        std::abs(
            result.doppler_bins -
            true_doppler) > 0.07 ||
        std::abs(
            result.gain -
            true_gain) > 0.15F) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
