#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/synchronization/chirp_synchronizer.hpp"
#include "otfs/core/types.hpp"

#include <iostream>
#include <cmath>
#include <cstdlib>
#include <numbers>
#include <random>
#include <vector>

int main()
{
    constexpr double sample_rate_hz = 1.92e6;
    constexpr std::size_t chirp_length = 1024U;
    constexpr std::size_t guard_length = 512U;
    constexpr std::size_t frame_length = 4864U;
    constexpr std::size_t prefix_length = 137U;
    constexpr std::size_t repetitions = 3U;
    constexpr double cfo_hz = 850.0;

    otfs::core::aligned_buffer<
        otfs::core::sample_t> chirp{
            chirp_length};

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
        guard_length +
        frame_length +
        guard_length;

    std::vector<otfs::core::sample_t> payload(
        payload_length,
        otfs::core::sample_t{});

    std::copy(
        chirp.begin(),
        chirp.end(),
        payload.begin());

    std::mt19937 generator{17U};
    std::normal_distribution<float> normal{
        0.0F,
        1.0F};

    const std::size_t frame_offset =
        chirp_length + guard_length;

    for (std::size_t index = 0;
         index < frame_length;
         ++index) {
        payload[frame_offset + index] = {
            normal(generator) /
                std::sqrt(2.0F),
            normal(generator) /
                std::sqrt(2.0F)
        };
    }

    const std::size_t capture_length =
        prefix_length +
        repetitions * payload_length;

    std::vector<otfs::core::sample_t> capture(
        capture_length,
        otfs::core::sample_t{});

    for (std::size_t index = 0;
         index < prefix_length;
         ++index) {
        capture[index] = {
            0.02F * normal(generator),
            0.02F * normal(generator)
        };
    }

    for (std::size_t repetition = 0;
         repetition < repetitions;
         ++repetition) {
        std::copy(
            payload.begin(),
            payload.end(),
            capture.begin() +
                static_cast<std::ptrdiff_t>(
                    prefix_length +
                    repetition * payload_length));
    }

    for (std::size_t index = 0;
         index < capture.size();
         ++index) {
        const double phase =
            2.0 *
            std::numbers::pi *
            cfo_hz *
            static_cast<double>(index) /
            sample_rate_hz;

        const otfs::core::sample_t phasor{
            static_cast<float>(
                std::cos(phase)),
            static_cast<float>(
                std::sin(phase))
        };

        capture[index] *= phasor;

        capture[index] += otfs::core::sample_t{
            0.02F * normal(generator),
            0.02F * normal(generator)
        };
    }

    otfs::core::chirp_sync_config config{
        .payload_length = payload_length,
        .minimum_peak_db = 10.0F,
        .minimum_rho = 0.25F
    };

    otfs::core::chirp_synchronizer synchronizer{
        chirp.span(),
        config,
        capture_length};

    const auto result =
        synchronizer.process(capture);
        
        std::cerr
    << "Signal result:"
    << " found=" << result.found
    << " start=" << result.chirp_start
    << " peak_index=" << result.matched_filter_peak
    << " peak_db=" << result.peak_db
    << " rho=" << result.rho
    << " peak_value=" << result.peak_value
    << '\n';

    if (!result.found) {
        return EXIT_FAILURE;
    }

    bool valid_start = false;

    for (std::size_t repetition = 0;
         repetition < repetitions;
         ++repetition) {
        const std::size_t expected =
            prefix_length +
            repetition * payload_length;

        const std::size_t timing_error =
            result.chirp_start > expected
                ? result.chirp_start - expected
                : expected - result.chirp_start;

        if (timing_error <= 2U) {
            valid_start = true;
            break;
        }
    }

    if (!valid_start ||
        result.rho < config.minimum_rho ||
        result.peak_db < config.minimum_peak_db ||
        result.matched_filter_peak !=
            result.chirp_start +
            chirp_length - 1U) {
        return EXIT_FAILURE;
    }

    std::vector<otfs::core::sample_t> noise_only(
        capture_length);

    for (auto& sample : noise_only) {
        sample = {
            0.02F * normal(generator),
            0.02F * normal(generator)
        };
    }

    const auto noise_result =
        synchronizer.process(noise_only);
        std::cerr
    << "Noise result:"
    << " found=" << noise_result.found
    << " start=" << noise_result.chirp_start
    << " peak_index=" << noise_result.matched_filter_peak
    << " peak_db=" << noise_result.peak_db
    << " rho=" << noise_result.rho
    << " peak_value=" << noise_result.peak_value
    << '\n';

    if (noise_result.found) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
