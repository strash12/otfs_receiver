#pragma once

#include "otfs/core/fft/fft_plan.hpp"
#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/model/channel.hpp"
#include "otfs/core/types.hpp"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <limits>
#include <numbers>
#include <span>
#include <stdexcept>

namespace otfs::core {

struct fid_operator_config {
    std::size_t frame_length{};
    double sample_rate_hz{0.96e6};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return frame_length > 0U &&
               sample_rate_hz > 0.0;
    }
};

class fid_operator final {
public:
    explicit fid_operator(
        const fid_operator_config& config)
        : config_(config),
          forward_(config.frame_length, fft_direction::forward),
          inverse_(config.frame_length, fft_direction::inverse),
          spectrum_(config.frame_length),
          delayed_spectrum_(config.frame_length),
          delayed_time_(config.frame_length),
          temporary_(config.frame_length)
    {
        if (!config_.valid()) {
            throw std::invalid_argument(
                "Invalid FID operator configuration");
        }
    }

    fid_operator(const fid_operator&) = delete;
    fid_operator& operator=(const fid_operator&) = delete;
    fid_operator(fid_operator&&) noexcept = default;
    fid_operator& operator=(fid_operator&&) noexcept = default;

    [[nodiscard]] std::size_t frame_length() const noexcept {
        return config_.frame_length;
    }

    [[nodiscard]] bool apply(
        std::span<const sample_t> input,
        std::span<const channel_path> paths,
        std::span<sample_t> output) noexcept
    {
        if (input.size() != frame_length() ||
            output.size() != frame_length()) {
            return false;
        }

        std::fill(output.begin(), output.end(), sample_t{});

        if (paths.empty()) {
            return true;
        }

        forward_.execute(
            input,
            spectrum_.span());

        for (const auto& path : paths) {
            if (!std::isfinite(path.delay_samples) ||
                !std::isfinite(path.doppler_bins)) {
                return false;
            }

            apply_delay_phase(
                spectrum_.span(),
                delayed_spectrum_.span(),
                path.delay_samples);

            inverse_.execute(
                delayed_spectrum_.span(),
                delayed_time_.span());

            for (std::size_t index = 0;
                 index < frame_length();
                 ++index) {
                const double phase =
                    2.0 *
                    std::numbers::pi *
                    path.doppler_bins *
                    static_cast<double>(index) /
                    static_cast<double>(frame_length());

                const sample_t phasor{
                    static_cast<float>(std::cos(phase)),
                    static_cast<float>(std::sin(phase))
                };

                output[index] +=
                    path.gain *
                    delayed_time_[index] *
                    phasor;
            }
        }

        return true;
    }

    [[nodiscard]] bool apply_adjoint(
        std::span<const sample_t> input,
        std::span<const channel_path> paths,
        std::span<sample_t> output) noexcept
    {
        if (input.size() != frame_length() ||
            output.size() != frame_length()) {
            return false;
        }

        std::fill(output.begin(), output.end(), sample_t{});

        for (const auto& path : paths) {
            if (!std::isfinite(path.delay_samples) ||
                !std::isfinite(path.doppler_bins)) {
                return false;
            }

            for (std::size_t index = 0;
                 index < frame_length();
                 ++index) {
                const double phase =
                    -2.0 *
                    std::numbers::pi *
                    path.doppler_bins *
                    static_cast<double>(index) /
                    static_cast<double>(frame_length());

                const sample_t phasor{
                    static_cast<float>(std::cos(phase)),
                    static_cast<float>(std::sin(phase))
                };

                temporary_[index] =
                    input[index] *
                    phasor;
            }

            forward_.execute(
                temporary_.span(),
                spectrum_.span());

            apply_delay_phase(
                spectrum_.span(),
                delayed_spectrum_.span(),
                -path.delay_samples);

            inverse_.execute(
                delayed_spectrum_.span(),
                delayed_time_.span());

            const sample_t conjugate_gain =
                std::conj(path.gain);

            for (std::size_t index = 0;
                 index < frame_length();
                 ++index) {
                output[index] +=
                    conjugate_gain *
                    delayed_time_[index];
            }
        }

        return true;
    }

private:
    void apply_delay_phase(
        std::span<const sample_t> input_spectrum,
        std::span<sample_t> output_spectrum,
        double delay_samples) noexcept
    {
        const auto length = frame_length();

        for (std::size_t index = 0;
             index < length;
             ++index) {
            std::ptrdiff_t signed_bin =
                static_cast<std::ptrdiff_t>(index);

            if (index >= length / 2U) {
                signed_bin -=
                    static_cast<std::ptrdiff_t>(length);
            }

            const double phase =
                -2.0 *
                std::numbers::pi *
                static_cast<double>(signed_bin) *
                delay_samples /
                static_cast<double>(length);

            const sample_t phasor{
                static_cast<float>(std::cos(phase)),
                static_cast<float>(std::sin(phase))
            };

            output_spectrum[index] =
                input_spectrum[index] *
                phasor;
        }
    }

    fid_operator_config config_{};

    fft_plan forward_;
    fft_plan inverse_;

    aligned_buffer<sample_t> spectrum_;
    aligned_buffer<sample_t> delayed_spectrum_;
    aligned_buffer<sample_t> delayed_time_;
    aligned_buffer<sample_t> temporary_;
};

struct spectral_norm_result {
    bool valid{};
    double norm{};
    std::size_t iterations{};
    double relative_change{};
};

class fid_spectral_norm_estimator final {
public:
    explicit fid_spectral_norm_estimator(
        std::size_t frame_length)
        : vector_(frame_length),
          applied_(frame_length),
          adjoint_applied_(frame_length)
    {
        if (frame_length == 0U) {
            throw std::invalid_argument(
                "Frame length must be positive");
        }
    }

    [[nodiscard]] spectral_norm_result estimate(
        fid_operator& op,
        std::span<const channel_path> paths,
        std::size_t maximum_iterations = 20U,
        double tolerance = 1e-5) noexcept
    {
        spectral_norm_result result{};

        if (maximum_iterations == 0U ||
            tolerance <= 0.0 ||
            vector_.size() != op.frame_length()) {
            return result;
        }

        const float scale =
            1.0F /
            std::sqrt(
                static_cast<float>(vector_.size()));

        for (auto& value : vector_) {
            value = {scale, 0.0F};
        }

        double previous = 0.0;

        for (std::size_t iteration = 0;
             iteration < maximum_iterations;
             ++iteration) {
            if (!op.apply(
                    vector_.span(),
                    paths,
                    applied_.span()) ||
                !op.apply_adjoint(
                    applied_.span(),
                    paths,
                    adjoint_applied_.span())) {
                return result;
            }

            double squared_norm = 0.0;

            for (const auto value :
                 adjoint_applied_) {
                squared_norm +=
                    static_cast<double>(
                        std::norm(value));
            }

            const double current =
                std::sqrt(squared_norm);

            if (!std::isfinite(current) ||
                current <=
                    std::numeric_limits<double>::epsilon()) {
                result.valid = paths.empty();
                result.norm = 0.0;
                result.iterations = iteration + 1U;
                return result;
            }

            for (std::size_t index = 0;
                 index < vector_.size();
                 ++index) {
                vector_[index] =
                    adjoint_applied_[index] /
                    static_cast<float>(current);
            }

            result.relative_change =
                previous > 0.0
                    ? std::abs(current - previous) /
                      previous
                    : 1.0;

            previous = current;
            result.iterations = iteration + 1U;

            if (result.relative_change <= tolerance) {
                break;
            }
        }

        if (!op.apply(
                vector_.span(),
                paths,
                applied_.span())) {
            return result;
        }

        double output_energy = 0.0;

        for (const auto value : applied_) {
            output_energy +=
                static_cast<double>(
                    std::norm(value));
        }

        result.norm = std::sqrt(output_energy);
        result.valid = std::isfinite(result.norm);
        return result;
    }

private:
    aligned_buffer<sample_t> vector_;
    aligned_buffer<sample_t> applied_;
    aligned_buffer<sample_t> adjoint_applied_;
};

} // namespace otfs::core
