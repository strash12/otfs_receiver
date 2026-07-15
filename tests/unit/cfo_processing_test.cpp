#include "otfs/core/cfo/cfo_corrector.hpp"
#include "otfs/core/cfo/cfo_estimator.hpp"
#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/types.hpp"

#include <cmath>
#include <cstdlib>
#include <numbers>
#include <random>

namespace {

[[nodiscard]] double relative_error(
    std::span<const otfs::core::sample_t> lhs,
    std::span<const otfs::core::sample_t> rhs)
{
    double numerator = 0.0;
    double denominator = 0.0;

    for (std::size_t index = 0;
         index < lhs.size();
         ++index) {
        numerator += static_cast<double>(
            std::norm(lhs[index] - rhs[index]));

        denominator += static_cast<double>(
            std::norm(rhs[index]));
    }

    return std::sqrt(
        numerator /
        std::max(denominator, 1e-30));
}

} // namespace

int main()
{
    constexpr double sample_rate_hz = 1.92e6;
    constexpr std::size_t block_length = 64U;
    constexpr std::size_t repetitions = 8U;
    constexpr std::size_t sample_count =
        block_length * repetitions;
    constexpr double true_cfo_hz = 600.0;

    otfs::core::aligned_buffer<
        otfs::core::sample_t> reference{
            sample_count};

    otfs::core::aligned_buffer<
        otfs::core::sample_t> impaired{
            sample_count};

    otfs::core::aligned_buffer<
        otfs::core::sample_t> corrected{
            sample_count};

    std::mt19937 generator{12345U};
    std::normal_distribution<float> noise{
        0.0F,
        0.002F};

    for (std::size_t index = 0;
         index < block_length;
         ++index) {
        const double phase =
            2.0 *
            std::numbers::pi *
            static_cast<double>(
                7U * index) /
            static_cast<double>(
                block_length);

        const otfs::core::sample_t value{
            static_cast<float>(
                std::cos(phase)),
            static_cast<float>(
                std::sin(phase))
        };

        for (std::size_t repetition = 0;
             repetition < repetitions;
             ++repetition) {
            reference[
                repetition * block_length +
                index] = value;
        }
    }

    for (std::size_t index = 0;
         index < sample_count;
         ++index) {
        const double phase =
            2.0 *
            std::numbers::pi *
            true_cfo_hz *
            static_cast<double>(index) /
            sample_rate_hz;

        const otfs::core::sample_t phasor{
            static_cast<float>(
                std::cos(phase)),
            static_cast<float>(
                std::sin(phase))
        };

        impaired[index] =
            reference[index] * phasor +
            otfs::core::sample_t{
                noise(generator),
                noise(generator)
            };
    }

    const otfs::core::repeated_block_cfo_config config{
        .sample_rate_hz = sample_rate_hz,
        .block_length = block_length,
        .repetitions = repetitions,
        .minimum_correlation = 1e-8
    };

    const otfs::core::repeated_block_cfo_estimator estimator{
        config};

    const auto estimate =
        estimator.estimate(impaired.span());

    if (!estimate.valid ||
        !std::isfinite(estimate.frequency_hz) ||
        std::abs(
            estimate.frequency_hz -
            true_cfo_hz) > 2.0 ||
        estimate.confidence < 0.99) {
        return EXIT_FAILURE;
    }

    otfs::core::cfo_corrector corrector{
        sample_rate_hz};

    corrector.reset();

    if (!corrector.process(
            impaired.span(),
            corrected.span(),
            estimate.frequency_hz)) {
        return EXIT_FAILURE;
    }

    const double error =
        relative_error(
            corrected.span(),
            reference.span());

    if (!std::isfinite(error) ||
        error > 0.01) {
        return EXIT_FAILURE;
    }

    const auto residual =
        estimator.estimate(corrected.span());

    if (!residual.valid ||
        std::abs(residual.frequency_hz) > 2.0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
