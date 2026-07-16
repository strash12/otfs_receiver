#pragma once

#include "otfs/core/channel_estimation/fractional_reference.hpp"
#include "otfs/core/channel_estimation/frft_local_search.hpp"
#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/model/channel.hpp"
#include "otfs/core/model/config.hpp"
#include "otfs/core/types.hpp"
#include "otfs/core/waveform/guard_interval.hpp"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <limits>
#include <span>
#include <stdexcept>

namespace otfs::core {

struct frft_sic_config {
    std::size_t maximum_paths{12};
    std::size_t refinement_passes{2};
    double minimum_correlation{0.15};
    double residual_ratio_stop{1e-3};
    double minimum_relative_power{1e-3};
    double ls_regularization{1e-6};
    frft_local_search_config local_search{};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return maximum_paths > 0U &&
               refinement_passes > 0U &&
               minimum_correlation >= 0.0 &&
               minimum_correlation <= 1.0 &&
               residual_ratio_stop >= 0.0 &&
               minimum_relative_power >= 0.0 &&
               ls_regularization >= 0.0 &&
               local_search.valid();
    }
};

struct frft_sic_result {
    bool valid{};
    std::size_t detected_paths{};
    std::size_t sic_iterations{};
    std::size_t refinement_passes{};
    std::size_t pruned_paths{};
    double initial_energy{};
    double residual_energy{};
    double residual_ratio{};
};

class frft_sic_estimator final {
public:
    frft_sic_estimator(
        std::span<const sample_t> pilot_time,
        const receiver_config& receiver_config,
        guard_interval guard_mode,
        const frft_sic_config& config)
        : config_(config),
          search_(
              pilot_time,
              receiver_config,
              guard_mode,
              config.local_search),
          reference_generator_(
              pilot_time,
              receiver_config,
              guard_mode),
          residual_(search_.observation_size()),
          temporary_(search_.observation_size()),
          reference_(search_.observation_size()),
          reference_bank_(
              config.maximum_paths *
              search_.observation_size()),
          matrix_(
              config.maximum_paths *
              config.maximum_paths),
          rhs_(config.maximum_paths),
          solution_(config.maximum_paths)
    {
        if (!receiver_config.valid() || !config_.valid()) {
            throw std::invalid_argument(
                "Invalid FRFT-SIC configuration");
        }
    }

    [[nodiscard]] std::size_t observation_size() const noexcept {
        return search_.observation_size();
    }

    [[nodiscard]] std::size_t maximum_paths() const noexcept {
        return config_.maximum_paths;
    }

    [[nodiscard]] frft_sic_result estimate(
        std::span<const sample_t> observation,
        std::span<const channel_path> seeds,
        std::span<channel_path> output) noexcept
    {
        frft_sic_result result{};

        if (observation.size() != observation_size() ||
            output.size() < maximum_paths()) {
            return result;
        }

        std::copy(
            observation.begin(),
            observation.end(),
            residual_.begin());

        result.initial_energy = energy(observation);

        if (result.initial_energy <=
            std::numeric_limits<double>::epsilon()) {
            result.valid = true;
            return result;
        }

        std::size_t detected = 0U;

        for (const auto& seed : seeds) {
            if (detected >= maximum_paths()) {
                break;
            }

            const auto found = search_.search(
                residual_.span(),
                seed.delay_samples,
                seed.doppler_bins);

            ++result.sic_iterations;

            if (!found.valid ||
                found.normalized_correlation <
                    config_.minimum_correlation) {
                continue;
            }

            output[detected] = {
                .gain = found.gain,
                .delay_samples = found.delay_samples,
                .doppler_bins = found.doppler_bins
            };

            if (!make_reference(
                    output[detected],
                    reference_.span())) {
                return result;
            }

            subtract(
                residual_.span(),
                reference_.span(),
                output[detected].gain);

            ++detected;

            if (energy(residual_.span()) /
                    result.initial_energy <=
                config_.residual_ratio_stop) {
                break;
            }
        }

        for (std::size_t pass = 0;
             pass < config_.refinement_passes &&
             detected > 0U;
             ++pass) {
            for (std::size_t current = 0;
                 current < detected;
                 ++current) {
                std::copy(
                    observation.begin(),
                    observation.end(),
                    temporary_.begin());

                for (std::size_t other = 0;
                     other < detected;
                     ++other) {
                    if (other == current) {
                        continue;
                    }

                    if (!make_reference(
                            output[other],
                            reference_.span())) {
                        return result;
                    }

                    subtract(
                        temporary_.span(),
                        reference_.span(),
                        output[other].gain);
                }

                const auto refined = search_.search(
                    temporary_.span(),
                    output[current].delay_samples,
                    output[current].doppler_bins);

                if (refined.valid &&
                    refined.normalized_correlation >=
                        config_.minimum_correlation) {
                    output[current] = {
                        .gain = refined.gain,
                        .delay_samples =
                            refined.delay_samples,
                        .doppler_bins =
                            refined.doppler_bins
                    };
                }
            }

            if (!joint_ls(
                    observation,
                    output.first(detected))) {
                return result;
            }

            ++result.refinement_passes;
        }

        detected = prune(
            output.first(detected),
            result.pruned_paths);

        rebuild_residual(
            observation,
            output.first(detected));

        result.detected_paths = detected;
        result.residual_energy =
            energy(residual_.span());
        result.residual_ratio =
            result.residual_energy /
            result.initial_energy;
        result.valid = true;
        return result;
    }

private:
    [[nodiscard]] double energy(
        std::span<const sample_t> values) const noexcept
    {
        double total = 0.0;

        for (const auto value : values) {
            total += static_cast<double>(
                std::norm(value));
        }

        return total;
    }

    [[nodiscard]] bool make_reference(
        const channel_path& path,
        std::span<sample_t> output) noexcept
    {
        return reference_generator_
            .make_observation_reference(
                path.delay_samples,
                path.doppler_bins,
                output);
    }

    static void subtract(
        std::span<sample_t> residual,
        std::span<const sample_t> reference,
        sample_t gain) noexcept
    {
        for (std::size_t index = 0;
             index < residual.size();
             ++index) {
            residual[index] -=
                gain * reference[index];
        }
    }

    [[nodiscard]] bool joint_ls(
        std::span<const sample_t> observation,
        std::span<channel_path> paths) noexcept
    {
        const std::size_t P = paths.size();
        const std::size_t Q = observation_size();

        for (std::size_t p = 0; p < P; ++p) {
            auto column =
                reference_bank_.span().subspan(
                    p * Q,
                    Q);

            if (!make_reference(paths[p], column)) {
                return false;
            }
        }

        for (std::size_t row = 0; row < P; ++row) {
            const auto row_ref =
                reference_bank_.span().subspan(
                    row * Q,
                    Q);

            accumulator_t b{};

            for (std::size_t q = 0; q < Q; ++q) {
                b += std::conj(
                         static_cast<accumulator_t>(
                             row_ref[q])) *
                     static_cast<accumulator_t>(
                         observation[q]);
            }

            rhs_[row] = b;

            for (std::size_t column = 0;
                 column < P;
                 ++column) {
                const auto column_ref =
                    reference_bank_.span().subspan(
                        column * Q,
                        Q);

                accumulator_t value{};

                for (std::size_t q = 0; q < Q; ++q) {
                    value += std::conj(
                                 static_cast<accumulator_t>(
                                     row_ref[q])) *
                             static_cast<accumulator_t>(
                                 column_ref[q]);
                }

                if (row == column) {
                    value += config_.ls_regularization;
                }

                matrix_[
                    row * maximum_paths() +
                    column] = value;
            }
        }

        if (!gaussian_elimination(P)) {
            return false;
        }

        for (std::size_t p = 0; p < P; ++p) {
            paths[p].gain = {
                static_cast<float>(
                    solution_[p].real()),
                static_cast<float>(
                    solution_[p].imag())
            };
        }

        return true;
    }

    [[nodiscard]] bool gaussian_elimination(
        std::size_t size) noexcept
    {
        const std::size_t stride =
            maximum_paths();

        for (std::size_t pivot = 0;
             pivot < size;
             ++pivot) {
            std::size_t best = pivot;

            for (std::size_t row = pivot + 1U;
                 row < size;
                 ++row) {
                if (std::abs(
                        matrix_[row * stride + pivot]) >
                    std::abs(
                        matrix_[best * stride + pivot])) {
                    best = row;
                }
            }

            if (std::abs(
                    matrix_[best * stride + pivot]) <
                1e-14) {
                return false;
            }

            if (best != pivot) {
                for (std::size_t column = 0;
                     column < size;
                     ++column) {
                    std::swap(
                        matrix_[
                            pivot * stride + column],
                        matrix_[
                            best * stride + column]);
                }
                std::swap(rhs_[pivot], rhs_[best]);
            }

            const auto diagonal =
                matrix_[pivot * stride + pivot];

            for (std::size_t column = pivot;
                 column < size;
                 ++column) {
                matrix_[
                    pivot * stride + column] /=
                    diagonal;
            }

            rhs_[pivot] /= diagonal;

            for (std::size_t row = 0;
                 row < size;
                 ++row) {
                if (row == pivot) {
                    continue;
                }

                const auto factor =
                    matrix_[row * stride + pivot];

                for (std::size_t column = pivot;
                     column < size;
                     ++column) {
                    matrix_[
                        row * stride + column] -=
                        factor *
                        matrix_[
                            pivot * stride + column];
                }

                rhs_[row] -=
                    factor * rhs_[pivot];
            }
        }

        for (std::size_t index = 0;
             index < size;
             ++index) {
            solution_[index] = rhs_[index];
        }

        return true;
    }

    [[nodiscard]] std::size_t prune(
        std::span<channel_path> paths,
        std::size_t& pruned) noexcept
    {
        double maximum = 0.0;

        for (const auto& path : paths) {
            maximum = std::max(
                maximum,
                static_cast<double>(
                    std::norm(path.gain)));
        }

        std::size_t write = 0U;

        for (std::size_t read = 0;
             read < paths.size();
             ++read) {
            const double power =
                static_cast<double>(
                    std::norm(paths[read].gain));

            if (maximum > 0.0 &&
                power <
                    maximum *
                    config_.minimum_relative_power) {
                ++pruned;
                continue;
            }

            paths[write++] = paths[read];
        }

        return write;
    }

    void rebuild_residual(
        std::span<const sample_t> observation,
        std::span<const channel_path> paths) noexcept
    {
        std::copy(
            observation.begin(),
            observation.end(),
            residual_.begin());

        for (const auto& path : paths) {
            if (make_reference(
                    path,
                    reference_.span())) {
                subtract(
                    residual_.span(),
                    reference_.span(),
                    path.gain);
            }
        }
    }

    frft_sic_config config_{};
    frft_local_search search_;
    fractional_reference_generator reference_generator_;

    aligned_buffer<sample_t> residual_;
    aligned_buffer<sample_t> temporary_;
    aligned_buffer<sample_t> reference_;
    aligned_buffer<sample_t> reference_bank_;
    aligned_buffer<accumulator_t> matrix_;
    aligned_buffer<accumulator_t> rhs_;
    aligned_buffer<accumulator_t> solution_;
};

} // namespace otfs::core
