#include "otfs/core/channel_estimation/fractional_peak_refiner.hpp"

#include "otfs/core/types.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace otfs::core {

fractional_peak_refiner::fractional_peak_refiner(
    const receiver_config& receiver_config,
    const fractional_peak_refiner_config& refiner_config)
    : receiver_config_(receiver_config),
      refiner_config_(refiner_config)
{
    if (!receiver_config_.valid()) {
        throw std::invalid_argument(
            "Invalid receiver configuration");
    }

    if (!refiner_config_.valid()) {
        throw std::invalid_argument(
            "Invalid fractional peak refiner configuration");
    }
}

fractional_refinement_result
fractional_peak_refiner::refine(
    std::span<const sample_t> normalized_delay_doppler,
    std::span<channel_path> paths) const noexcept
{
    fractional_refinement_result result{};

    if (normalized_delay_doppler.size() !=
        receiver_config_.waveform.dd_cells()) {
        return result;
    }

    const auto M =
        receiver_config_.waveform.delay_bins;

    const auto N =
        receiver_config_.waveform.doppler_bins;

    for (auto& path : paths) {
        const double absolute_delay =
            static_cast<double>(
                receiver_config_.pilot.delay_index) +
            path.delay_samples;

        const double absolute_doppler =
            static_cast<double>(
                receiver_config_.pilot.doppler_index) +
            path.doppler_bins;

        const auto delay_center_signed =
            static_cast<std::ptrdiff_t>(
                std::llround(absolute_delay));

        const auto doppler_center_signed =
            static_cast<std::ptrdiff_t>(
                std::llround(absolute_doppler));

        if (delay_center_signed < 0 ||
            delay_center_signed >=
                static_cast<std::ptrdiff_t>(M)) {
            continue;
        }

        const auto delay_center =
            static_cast<std::size_t>(
                delay_center_signed);

        const auto doppler_center =
            wrap_doppler(
                doppler_center_signed);

        double delay_offset = 0.0;
        double doppler_offset = 0.0;

        bool delay_refined = false;
        bool doppler_refined = false;

        if (refiner_config_.refine_delay &&
            delay_center > 0U &&
            delay_center + 1U < M) {
            const double left =
                power_at(
                    normalized_delay_doppler,
                    delay_center - 1U,
                    doppler_center);

            const double center =
                power_at(
                    normalized_delay_doppler,
                    delay_center,
                    doppler_center);

            const double right =
                power_at(
                    normalized_delay_doppler,
                    delay_center + 1U,
                    doppler_center);

            delay_offset =
                parabolic_offset(
                    left,
                    center,
                    right);

            delay_refined =
                std::abs(delay_offset) > 0.0;
        }

        if (refiner_config_.refine_doppler) {
            const auto left_doppler =
                wrap_doppler(
                    doppler_center_signed - 1);

            const auto right_doppler =
                wrap_doppler(
                    doppler_center_signed + 1);

            const double left =
                power_at(
                    normalized_delay_doppler,
                    delay_center,
                    left_doppler);

            const double center =
                power_at(
                    normalized_delay_doppler,
                    delay_center,
                    doppler_center);

            const double right =
                power_at(
                    normalized_delay_doppler,
                    delay_center,
                    right_doppler);

            doppler_offset =
                parabolic_offset(
                    left,
                    center,
                    right);

            doppler_refined =
                std::abs(doppler_offset) > 0.0;
        }

        const double refined_absolute_delay =
            static_cast<double>(delay_center) +
            delay_offset;

        double refined_absolute_doppler =
            static_cast<double>(doppler_center) +
            doppler_offset;

        const double N_as_double =
            static_cast<double>(N);

        while (refined_absolute_doppler >= N_as_double) {
            refined_absolute_doppler -= N_as_double;
        }

        while (refined_absolute_doppler < 0.0) {
            refined_absolute_doppler += N_as_double;
        }

        path.delay_samples =
            refined_absolute_delay -
            static_cast<double>(
                receiver_config_.pilot.delay_index);

        path.doppler_bins =
            refined_absolute_doppler -
            static_cast<double>(
                receiver_config_.pilot.doppler_index);

        const double half_grid =
            0.5 * N_as_double;

        if (path.doppler_bins > half_grid) {
            path.doppler_bins -= N_as_double;
        }
        else if (path.doppler_bins < -half_grid) {
            path.doppler_bins += N_as_double;
        }

        ++result.refined_paths;

        if (delay_refined) {
            ++result.delay_refinements;
        }

        if (doppler_refined) {
            ++result.doppler_refinements;
        }
    }

    result.valid = true;
    return result;
}

double fractional_peak_refiner::power_at(
    std::span<const sample_t> grid,
    std::size_t delay,
    std::size_t doppler) const noexcept
{
    const auto index =
        column_major_index(
            delay,
            doppler,
            receiver_config_.waveform.delay_bins);

    return static_cast<double>(
        std::norm(grid[index]));
}

double fractional_peak_refiner::parabolic_offset(
    double left,
    double center,
    double right) const noexcept
{
    const double denominator =
        left - 2.0 * center + right;

    if (!std::isfinite(denominator) ||
        std::abs(denominator) <
            refiner_config_.minimum_curvature ||
        denominator >= 0.0) {
        return 0.0;
    }

    const double offset =
        0.5 * (left - right) /
        denominator;

    if (!std::isfinite(offset)) {
        return 0.0;
    }

    return std::clamp(
        offset,
        -refiner_config_.maximum_fractional_offset,
        refiner_config_.maximum_fractional_offset);
}

std::size_t fractional_peak_refiner::wrap_doppler(
    std::ptrdiff_t doppler) const noexcept
{
    const auto N =
        static_cast<std::ptrdiff_t>(
            receiver_config_.waveform.doppler_bins);

    doppler %= N;

    if (doppler < 0) {
        doppler += N;
    }

    return static_cast<std::size_t>(doppler);
}

} // namespace otfs::core
