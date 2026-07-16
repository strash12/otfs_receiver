#pragma once

#include "otfs/core/channel/fid_operator.hpp"
#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/model/channel.hpp"
#include "otfs/core/types.hpp"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <limits>
#include <span>
#include <stdexcept>

namespace otfs::core {

struct cg_lmmse_config {
    std::size_t maximum_iterations{40};
    double tolerance{1e-5};
    double lambda_floor{1e-8};
    double estimation_error_scale{1.0};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return maximum_iterations > 0U &&
               tolerance > 0.0 &&
               lambda_floor >= 0.0 &&
               estimation_error_scale >= 0.0;
    }
};

struct cg_lmmse_result {
    bool valid{};
    bool converged{};
    bool breakdown{};
    std::size_t iterations{};
    double relative_residual{};
    double lambda{};
};

class cg_lmmse_equalizer final {
public:
    cg_lmmse_equalizer(
        std::size_t frame_length,
        const cg_lmmse_config& config)
        : config_(config),
          rhs_(frame_length),
          residual_(frame_length),
          direction_(frame_length),
          normal_direction_(frame_length),
          temporary_(frame_length)
    {
        if (frame_length == 0U || !config_.valid()) {
            throw std::invalid_argument(
                "Invalid CG-LMMSE configuration");
        }
    }

    [[nodiscard]] static double adaptive_lambda(
        double noise_variance,
        double channel_error_variance,
        const cg_lmmse_config& config) noexcept
    {
        return std::max({
            config.lambda_floor,
            noise_variance,
            config.estimation_error_scale *
                channel_error_variance
        });
    }

    [[nodiscard]] cg_lmmse_result equalize(
        fid_operator& op,
        std::span<const channel_path> paths,
        std::span<const sample_t> received,
        std::span<sample_t> output,
        double noise_variance,
        double channel_error_variance) noexcept
    {
        cg_lmmse_result result{};

        if (received.size() != output.size() ||
            received.size() != rhs_.size()) {
            return result;
        }

        result.lambda = adaptive_lambda(
            noise_variance,
            channel_error_variance,
            config_);

        if (!op.apply_adjoint(
                received,
                paths,
                rhs_.span())) {
            return result;
        }

        std::fill(
            output.begin(),
            output.end(),
            sample_t{});

        std::copy(
            rhs_.begin(),
            rhs_.end(),
            residual_.begin());

        std::copy(
            residual_.begin(),
            residual_.end(),
            direction_.begin());

        double rhs_norm_squared = 0.0;
        double residual_norm_squared = 0.0;

        for (const auto value : rhs_) {
            rhs_norm_squared +=
                static_cast<double>(std::norm(value));
        }

        residual_norm_squared = rhs_norm_squared;

        if (rhs_norm_squared <=
            std::numeric_limits<double>::epsilon()) {
            result.valid = true;
            result.converged = true;
            return result;
        }

        for (std::size_t iteration = 0;
             iteration < config_.maximum_iterations;
             ++iteration) {
            if (!apply_normal(
                    op,
                    paths,
                    direction_.span(),
                    normal_direction_.span(),
                    result.lambda)) {
                return result;
            }

            accumulator_t curvature{};

            for (std::size_t index = 0;
                 index < direction_.size();
                 ++index) {
                curvature +=
                    std::conj(
                        static_cast<accumulator_t>(
                            direction_[index])) *
                    static_cast<accumulator_t>(
                        normal_direction_[index]);
            }

            const double denominator =
                curvature.real();

            if (!std::isfinite(denominator) ||
                denominator <= 1e-20) {
                result.breakdown = true;
                break;
            }

            const double alpha =
                residual_norm_squared /
                denominator;

            for (std::size_t index = 0;
                 index < output.size();
                 ++index) {
                output[index] +=
                    static_cast<float>(alpha) *
                    direction_[index];

                residual_[index] -=
                    static_cast<float>(alpha) *
                    normal_direction_[index];
            }

            double next_residual_norm_squared = 0.0;

            for (const auto value : residual_) {
                next_residual_norm_squared +=
                    static_cast<double>(
                        std::norm(value));
            }

            result.iterations = iteration + 1U;
            result.relative_residual =
                std::sqrt(
                    next_residual_norm_squared /
                    rhs_norm_squared);

            if (result.relative_residual <=
                config_.tolerance) {
                result.converged = true;
                residual_norm_squared =
                    next_residual_norm_squared;
                break;
            }

            const double beta =
                next_residual_norm_squared /
                residual_norm_squared;

            for (std::size_t index = 0;
                 index < direction_.size();
                 ++index) {
                direction_[index] =
                    residual_[index] +
                    static_cast<float>(beta) *
                    direction_[index];
            }

            residual_norm_squared =
                next_residual_norm_squared;
        }

        result.valid =
            !result.breakdown &&
            std::isfinite(result.relative_residual);

        return result;
    }

private:
    [[nodiscard]] bool apply_normal(
        fid_operator& op,
        std::span<const channel_path> paths,
        std::span<const sample_t> input,
        std::span<sample_t> output,
        double lambda) noexcept
    {
        if (!op.apply(
                input,
                paths,
                temporary_.span()) ||
            !op.apply_adjoint(
                temporary_.span(),
                paths,
                output)) {
            return false;
        }

        for (std::size_t index = 0;
             index < output.size();
             ++index) {
            output[index] +=
                static_cast<float>(lambda) *
                input[index];
        }

        return true;
    }

    cg_lmmse_config config_{};

    aligned_buffer<sample_t> rhs_;
    aligned_buffer<sample_t> residual_;
    aligned_buffer<sample_t> direction_;
    aligned_buffer<sample_t> normal_direction_;
    aligned_buffer<sample_t> temporary_;
};

} // namespace otfs::core
