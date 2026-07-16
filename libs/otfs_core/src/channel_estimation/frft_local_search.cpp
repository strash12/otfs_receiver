#include "otfs/core/channel_estimation/frft_local_search.hpp"

#include <algorithm>
#include <cmath>
#include <complex>
#include <limits>
#include <stdexcept>

namespace otfs::core {

frft_local_search::frft_local_search(
    std::span<const sample_t> pilot_time,
    const receiver_config& receiver_config,
    guard_interval guard_mode,
    const frft_local_search_config& search_config)
    : reference_generator_(
          pilot_time,
          receiver_config,
          guard_mode),
      config_(search_config),
      candidate_reference_(
          reference_generator_.observation_size()),
      best_reference_(
          reference_generator_.observation_size()),
      metric_(
          search_config.fine_points *
          search_config.fine_points)
{
    if (!config_.valid()) {
        throw std::invalid_argument(
            "Invalid FRFT local search configuration");
    }
}

frft_local_search_result frft_local_search::search(
    std::span<const sample_t> residual_observation,
    double center_delay,
    double center_doppler) noexcept
{
    frft_local_search_result result{};

    if (residual_observation.size() !=
            observation_size() ||
        !std::isfinite(center_delay) ||
        !std::isfinite(center_doppler)) {
        return result;
    }

    double residual_energy = 0.0;

    for (const auto sample : residual_observation) {
        residual_energy +=
            static_cast<double>(std::norm(sample));
    }

    if (residual_energy <=
        std::numeric_limits<double>::epsilon()) {
        return result;
    }

    std::size_t best_row = 0U;
    std::size_t best_column = 0U;
    double best_metric = -1.0;
    double best_reference_energy = 0.0;

    for (std::size_t delay_index = 0;
         delay_index < config_.fine_points;
         ++delay_index) {
        const double delay =
            grid_value(
                center_delay,
                config_.delay_radius,
                delay_index);

        for (std::size_t doppler_index = 0;
             doppler_index < config_.fine_points;
             ++doppler_index) {
            const double doppler =
                grid_value(
                    center_doppler,
                    config_.doppler_radius,
                    doppler_index);

            if (!reference_generator_
                    .make_observation_reference(
                        delay,
                        doppler,
                        candidate_reference_.span())) {
                return result;
            }

            accumulator_t correlation{};
            double reference_energy = 0.0;

            for (std::size_t position = 0;
                 position < observation_size();
                 ++position) {
                correlation +=
                    std::conj(
                        static_cast<accumulator_t>(
                            candidate_reference_[position])) *
                    static_cast<accumulator_t>(
                        residual_observation[position]);

                reference_energy +=
                    static_cast<double>(
                        std::norm(
                            candidate_reference_[position]));
            }

            const double value =
                std::abs(correlation);

            const std::size_t metric_index =
                delay_index *
                config_.fine_points +
                doppler_index;

            metric_[metric_index] = value;

            if (value > best_metric) {
                best_metric = value;
                best_row = delay_index;
                best_column = doppler_index;
                best_reference_energy =
                    reference_energy;

                std::copy(
                    candidate_reference_.begin(),
                    candidate_reference_.end(),
                    best_reference_.begin());
            }
        }
    }

    const double refined_delay =
        parabolic_coordinate(
            metric_.span(),
            best_row,
            best_column,
            true,
            center_delay,
            center_doppler);

    const double refined_doppler =
        parabolic_coordinate(
            metric_.span(),
            best_row,
            best_column,
            false,
            center_delay,
            center_doppler);

    if (!reference_generator_
            .make_observation_reference(
                refined_delay,
                refined_doppler,
                best_reference_.span())) {
        return result;
    }

    double refined_reference_energy = 0.0;
    accumulator_t refined_correlation{};

    for (std::size_t position = 0;
         position < observation_size();
         ++position) {
        refined_correlation +=
            std::conj(
                static_cast<accumulator_t>(
                    best_reference_[position])) *
            static_cast<accumulator_t>(
                residual_observation[position]);

        refined_reference_energy +=
            static_cast<double>(
                std::norm(best_reference_[position]));
    }

    result.delay_samples = refined_delay;
    result.doppler_bins = refined_doppler;
    result.gain =
        estimate_gain(
            best_reference_.span(),
            residual_observation);

    result.peak_metric =
        std::abs(refined_correlation);

    result.normalized_correlation =
        result.peak_metric /
        (std::sqrt(
             refined_reference_energy *
             residual_energy) +
         std::numeric_limits<double>::epsilon());

    result.evaluated_candidates =
        config_.fine_points *
        config_.fine_points;

    result.valid =
        std::isfinite(result.delay_samples) &&
        std::isfinite(result.doppler_bins) &&
        std::isfinite(result.peak_metric) &&
        std::isfinite(
            result.normalized_correlation);

    return result;
}

std::size_t
frft_local_search::observation_size() const noexcept
{
    return reference_generator_.observation_size();
}

double frft_local_search::grid_value(
    double center,
    double radius,
    std::size_t index) const noexcept
{
    if (config_.fine_points <= 1U) {
        return center;
    }

    const double step =
        2.0 * radius /
        static_cast<double>(
            config_.fine_points - 1U);

    return center - radius +
           step * static_cast<double>(index);
}

double frft_local_search::parabolic_coordinate(
    std::span<const double> metric,
    std::size_t row,
    std::size_t column,
    bool delay_axis,
    double center_delay,
    double center_doppler) const noexcept
{
    const std::size_t size =
        config_.fine_points;

    const auto at =
        [&](std::size_t r,
            std::size_t c) noexcept {
            return metric[r * size + c];
        };

    const double center_coordinate =
        delay_axis
            ? grid_value(
                  center_delay,
                  config_.delay_radius,
                  row)
            : grid_value(
                  center_doppler,
                  config_.doppler_radius,
                  column);

    const std::size_t axis_index =
        delay_axis ? row : column;

    if (axis_index == 0U ||
        axis_index + 1U >= size) {
        return center_coordinate;
    }

    const double left =
        delay_axis
            ? at(row - 1U, column)
            : at(row, column - 1U);

    const double center =
        at(row, column);

    const double right =
        delay_axis
            ? at(row + 1U, column)
            : at(row, column + 1U);

    const double denominator =
        left - 2.0 * center + right;

    if (!std::isfinite(denominator) ||
        std::abs(denominator) <
            config_.minimum_curvature) {
        return center_coordinate;
    }

    const double step =
        2.0 *
        (delay_axis
             ? config_.delay_radius
             : config_.doppler_radius) /
        static_cast<double>(size - 1U);

    const double offset =
        std::clamp(
            0.5 *
                (left - right) /
                denominator,
            -1.0,
            1.0);

    return center_coordinate +
           offset * step;
}

sample_t frft_local_search::estimate_gain(
    std::span<const sample_t> reference,
    std::span<const sample_t> residual) const noexcept
{
    accumulator_t numerator{};
    double denominator = 0.0;

    for (std::size_t index = 0;
         index < reference.size();
         ++index) {
        numerator +=
            std::conj(
                static_cast<accumulator_t>(
                    reference[index])) *
            static_cast<accumulator_t>(
                residual[index]);

        denominator +=
            static_cast<double>(
                std::norm(reference[index]));
    }

    if (denominator <=
        std::numeric_limits<double>::epsilon()) {
        return {};
    }

    const accumulator_t gain =
        numerator / denominator;

    return {
        static_cast<float>(gain.real()),
        static_cast<float>(gain.imag())
    };
}

} // namespace otfs::core
