#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/synchronization/synchronization_pipeline.hpp"
#include "otfs/core/types.hpp"

#include <cmath>
#include <cstdlib>
#include <numbers>
#include <random>
#include <vector>

int main()
{
    constexpr double sample_rate_hz = 1.92e6;
    constexpr std::size_t chirp_length = 1024U;
    constexpr std::size_t cfo_block_length = 64U;
    constexpr std::size_t cfo_repetitions = 8U;
    constexpr std::size_t cfo_length =
        cfo_block_length * cfo_repetitions;
    constexpr std::size_t guard_length = 512U;
    constexpr std::size_t frame_length = 4864U;
    constexpr std::size_t prefix_length = 211U;
    constexpr double true_cfo_hz = 603.5;

    otfs::core::aligned_buffer<
        otfs::core::sample_t> chirp{chirp_length};

    for (std::size_t index = 0;
         index < chirp_length;
         ++index) {
        const double n =
            static_cast<double>(index);

        const double phase =
            std::numbers::pi *
            (-0.70 * n +
             0.70 * n * n /
             static_cast<double>(
                 chirp_length - 1U));

        chirp[index] = {
            static_cast<float>(
                std::cos(phase)),
            static_cast<float>(
                std::sin(phase))
        };
    }

    const std::size_t payload_length =
        chirp_length +
        cfo_length +
        guard_length +
        frame_length +
        guard_length;

    const std::size_t capture_length =
        prefix_length + payload_length;

    std::vector<otfs::core::sample_t> capture(
        capture_length,
        otfs::core::sample_t{});

    std::copy(
        chirp.begin(),
        chirp.end(),
        capture.begin() +
            static_cast<std::ptrdiff_t>(
                prefix_length));

    std::mt19937 generator{42U};
    std::normal_distribution<float> noise{
        0.0F,
        0.01F};

    std::vector<otfs::core::sample_t> repeated_block(
        cfo_block_length);

    for (std::size_t index = 0;
         index < cfo_block_length;
         ++index) {
        const double phase =
            2.0 * std::numbers::pi *
            9.0 *
            static_cast<double>(index) /
            static_cast<double>(
                cfo_block_length);

        repeated_block[index] = {
            static_cast<float>(
                std::cos(phase)),
            static_cast<float>(
                std::sin(phase))
        };
    }

    const std::size_t cfo_start =
        prefix_length + chirp_length;

    for (std::size_t repetition = 0;
         repetition < cfo_repetitions;
         ++repetition) {
        std::copy(
            repeated_block.begin(),
            repeated_block.end(),
            capture.begin() +
                static_cast<std::ptrdiff_t>(
                    cfo_start +
                    repetition *
                    cfo_block_length));
    }

    for (std::size_t index =
             cfo_start + cfo_length + guard_length;
         index < capture_length - guard_length;
         ++index) {
        capture[index] = {
            noise(generator),
            noise(generator)
        };
    }

    for (std::size_t index = 0;
         index < capture.size();
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

        capture[index] =
            capture[index] * phasor +
            otfs::core::sample_t{
                noise(generator),
                noise(generator)
            };
    }

    otfs::core::synchronization_pipeline_config config{
        .chirp = {
            .payload_length = payload_length,
            .minimum_peak_db = 10.0F,
            .minimum_rho = 0.25F
        },
        .coarse_cfo = {
            .sample_rate_hz = sample_rate_hz,
            .block_length = cfo_block_length,
            .repetitions = cfo_repetitions,
            .minimum_correlation = 1e-8
        },
        .residual_cfo = {
            .sample_rate_hz = sample_rate_hz,
            .block_length = cfo_block_length,
            .repetitions = cfo_repetitions,
            .minimum_correlation = 1e-8
        },
        .cfo_block_offset = chirp_length,
        .maximum_capture_length = capture_length
    };

    otfs::core::synchronization_pipeline pipeline{
        chirp.span(),
        config};

    otfs::core::aligned_buffer<
        otfs::core::sample_t> corrected{
            capture_length};

    const auto result =
        pipeline.process(
            capture,
            corrected.span());

    if (!result.valid) {
        return EXIT_FAILURE;
    }

    const std::size_t timing_error =
        result.refined_sync.chirp_start >
            prefix_length
            ? result.refined_sync.chirp_start -
                prefix_length
            : prefix_length -
                result.refined_sync.chirp_start;

    if (timing_error > 2U) {
        return EXIT_FAILURE;
    }

    if (std::abs(
            result.total_cfo_hz -
            true_cfo_hz) > 2.0) {
        return EXIT_FAILURE;
    }

    if (std::abs(
            result.residual_cfo.frequency_hz) >
        2.0) {
        return EXIT_FAILURE;
    }

    if (result.coarse_cfo.confidence < 0.99 ||
        result.residual_cfo.confidence < 0.99) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
