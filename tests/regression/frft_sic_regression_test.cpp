#include "otfs/core/channel_estimation/frft_sic_estimator.hpp"
#include "otfs/core/model/channel.hpp"
#include "otfs/core/model/config.hpp"
#include "otfs/core/types.hpp"
#include "otfs/core/waveform/guard_interval.hpp"
#include "otfs/io/binary_array.hpp"
#include "otfs/io/reference_data.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <span>
#include <vector>

namespace {

[[nodiscard]] bool path_matches(
    const otfs::core::channel_path& estimated,
    double delay,
    double doppler,
    otfs::core::sample_t gain)
{
    return
        std::abs(estimated.delay_samples - delay) < 0.15 &&
        std::abs(estimated.doppler_bins - doppler) < 0.18 &&
        std::abs(estimated.gain - gain) < 0.18F;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr
            << "Usage: frft_sic_regression_test <reference-dir>\n";
        return 2;
    }

    const std::filesystem::path directory{argv[1]};

    if (!std::filesystem::exists(
            directory / "metadata.txt")) {
        std::cout
            << "SKIP: FRFT-SIC MATLAB references not generated.\n";
        return 77;
    }

    std::vector<otfs::core::sample_t> pilot_time;
    std::vector<otfs::core::sample_t> observation;
    std::vector<otfs::core::sample_t> gains;
    std::vector<double> delays;
    std::vector<double> dopplers;

    if (!otfs::io::read_complex_float32_le(
            directory / "pilot_time_zp.cf32",
            pilot_time) ||
        !otfs::io::read_complex_float32_le(
            directory / "observation.cf32",
            observation) ||
        !otfs::io::read_complex_float32_le(
            directory / "gains.cf32",
            gains) ||
        !otfs::io::read_float64_le(
            directory / "delays.f64",
            delays) ||
        !otfs::io::read_float64_le(
            directory / "dopplers.f64",
            dopplers)) {
        std::cerr << "Failed to read MATLAB FRFT-SIC dataset\n";
        return EXIT_FAILURE;
    }

    if (delays.size() != 3U ||
        dopplers.size() != 3U ||
        gains.size() != 3U) {
        return EXIT_FAILURE;
    }

    constexpr otfs::core::receiver_config receiver_config{};

    const otfs::core::frft_sic_config config{
        .maximum_paths = 6U,
        .refinement_passes = 2U,
        .minimum_correlation = 0.08,
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
        pilot_time,
        receiver_config,
        otfs::core::guard_interval::zero_padding,
        config};

    const otfs::core::channel_path seeds[] = {
        {{}, 2.0,  0.0},
        {{}, 7.0,  1.0},
        {{}, 10.0, -2.0}
    };

    std::vector<otfs::core::channel_path> estimated(
        estimator.maximum_paths());

    const auto result =
        estimator.estimate(
            observation,
            seeds,
            estimated);

    std::cout
        << "FRFT-SIC regression:"
        << " detected=" << result.detected_paths
        << " residual_ratio=" << result.residual_ratio
        << " passes=" << result.refinement_passes
        << '\n';

    if (!result.valid ||
        result.detected_paths != 3U ||
        result.refinement_passes != 2U ||
        result.residual_ratio > 2e-2) {
        return EXIT_FAILURE;
    }

    for (std::size_t truth = 0;
         truth < 3U;
         ++truth) {
        bool found = false;

        for (std::size_t candidate = 0;
             candidate < result.detected_paths;
             ++candidate) {
            if (path_matches(
                    estimated[candidate],
                    delays[truth],
                    dopplers[truth],
                    gains[truth])) {
                found = true;
                break;
            }
        }

        if (!found) {
            std::cerr
                << "Missing MATLAB path " << truth
                << ": delay=" << delays[truth]
                << ", doppler=" << dopplers[truth]
                << ", gain=" << gains[truth]
                << '\n';
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
