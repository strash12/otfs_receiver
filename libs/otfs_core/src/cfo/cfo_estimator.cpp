#include "otfs/core/cfo/cfo_estimator.hpp"

#include <cmath>
#include <complex>
#include <limits>
#include <stdexcept>

namespace otfs::core {

repeated_block_cfo_estimator::repeated_block_cfo_estimator(
    const repeated_block_cfo_config& config)
    : config_(config)
{
    if (!config_.valid()) {
        throw std::invalid_argument(
            "Invalid repeated-block CFO configuration");
    }
}

cfo_estimate repeated_block_cfo_estimator::estimate(
    std::span<const sample_t> samples) const noexcept
{
    cfo_estimate result{};

    const std::size_t required =
        config_.block_length * config_.repetitions;

    if (samples.size() < required) {
        return result;
    }

    accumulator_t correlation{};
    double reference_energy = 0.0;
    double delayed_energy = 0.0;

    for (std::size_t repetition = 0;
         repetition + 1U < config_.repetitions;
         ++repetition) {
        const std::size_t first_offset =
            repetition * config_.block_length;

        const std::size_t second_offset =
            first_offset + config_.block_length;

        for (std::size_t index = 0;
             index < config_.block_length;
             ++index) {
            const auto first =
                samples[first_offset + index];

            const auto second =
                samples[second_offset + index];

            correlation +=
                std::conj(
                    static_cast<accumulator_t>(first)) *
                static_cast<accumulator_t>(second);

            reference_energy +=
                static_cast<double>(
                    std::norm(first));

            delayed_energy +=
                static_cast<double>(
                    std::norm(second));
        }
    }

    const double magnitude =
        std::abs(correlation);

    const double denominator =
        std::sqrt(
            reference_energy *
            delayed_energy) +
        std::numeric_limits<double>::epsilon();

    const double confidence =
        magnitude / denominator;

    if (!std::isfinite(magnitude) ||
        magnitude < config_.minimum_correlation ||
        !std::isfinite(confidence)) {
        return result;
    }

    const double phase =
        std::arg(correlation);

    const double frequency_hz =
        phase *
        config_.sample_rate_hz /
        (2.0 *
         std::numbers::pi *
         static_cast<double>(
             config_.block_length));

    result.valid = true;
    result.frequency_hz = frequency_hz;
    result.normalized_frequency =
        frequency_hz / config_.sample_rate_hz;
    result.confidence = confidence;
    result.correlation_magnitude = magnitude;

    return result;
}

const repeated_block_cfo_config&
repeated_block_cfo_estimator::config() const noexcept
{
    return config_;
}

} // namespace otfs::core
