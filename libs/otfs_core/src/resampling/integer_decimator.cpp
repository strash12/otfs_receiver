#include "otfs/core/resampling/integer_decimator.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace otfs::core {

namespace {

[[nodiscard]] double sinc(double value) noexcept
{
    if (std::abs(value) < 1e-12) {
        return 1.0;
    }

    return std::sin(
               std::numbers::pi * value) /
           (std::numbers::pi * value);
}

} // namespace

integer_decimator::integer_decimator(
    const integer_decimator_config& config)
    : config_(config),
      coefficients_(config.taps)
{
    if (!config_.valid()) {
        throw std::invalid_argument(
            "Invalid integer decimator configuration");
    }

    if (config_.factor == 1U) {
        coefficients_.fill(0.0F);
        coefficients_[config_.taps / 2U] = 1.0F;
        return;
    }

    const std::size_t center =
        config_.taps / 2U;

    const double cutoff =
        0.5 *
        config_.cutoff_ratio /
        static_cast<double>(config_.factor);

    double sum = 0.0;

    for (std::size_t index = 0;
         index < config_.taps;
         ++index) {
        const double offset =
            static_cast<double>(index) -
            static_cast<double>(center);

        const double ideal =
            2.0 * cutoff *
            sinc(2.0 * cutoff * offset);

        const double window =
            0.54 -
            0.46 *
            std::cos(
                2.0 *
                std::numbers::pi *
                static_cast<double>(index) /
                static_cast<double>(
                    config_.taps - 1U));

        const double value =
            ideal * window;

        coefficients_[index] =
            static_cast<float>(value);

        sum += value;
    }

    if (std::abs(sum) < 1e-15) {
        throw std::runtime_error(
            "Decimator coefficient normalization failed");
    }

    for (auto& coefficient : coefficients_) {
        coefficient =
            static_cast<float>(
                static_cast<double>(coefficient) /
                sum);
    }
}

std::size_t integer_decimator::factor() const noexcept
{
    return config_.factor;
}

std::size_t integer_decimator::taps() const noexcept
{
    return config_.taps;
}

std::size_t integer_decimator::output_size(
    std::size_t input_size) const noexcept
{
    return input_size % config_.factor == 0U
        ? input_size / config_.factor
        : 0U;
}

bool integer_decimator::process_frame(
    std::span<const sample_t> input,
    std::span<sample_t> output) const noexcept
{
    const std::size_t expected =
        output_size(input.size());

    if (expected == 0U ||
        output.size() != expected) {
        return false;
    }

    if (config_.factor == 1U) {
        std::copy(
            input.begin(),
            input.end(),
            output.begin());
        return true;
    }

    const std::ptrdiff_t center =
        static_cast<std::ptrdiff_t>(
            config_.taps / 2U);

    for (std::size_t output_index = 0;
         output_index < output.size();
         ++output_index) {
        const std::ptrdiff_t input_center =
            static_cast<std::ptrdiff_t>(
                output_index *
                config_.factor);

        accumulator_t accumulated{};

        for (std::size_t tap = 0;
             tap < config_.taps;
             ++tap) {
            const std::ptrdiff_t input_index =
                input_center +
                static_cast<std::ptrdiff_t>(tap) -
                center;

            if (input_index < 0 ||
                input_index >=
                    static_cast<std::ptrdiff_t>(
                        input.size())) {
                continue;
            }

            accumulated +=
                static_cast<accumulator_t>(
                    input[
                        static_cast<std::size_t>(
                            input_index)]) *
                static_cast<double>(
                    coefficients_[tap]);
        }

        output[output_index] = {
            static_cast<float>(
                accumulated.real()),
            static_cast<float>(
                accumulated.imag())
        };
    }

    return true;
}

std::span<const float>
integer_decimator::coefficients() const noexcept
{
    return coefficients_.span();
}

} // namespace otfs::core
