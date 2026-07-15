#include "otfs/core/framing/frame_extractor.hpp"
#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/resampling/integer_decimator.hpp"
#include "otfs/core/resampling/sample_rate_config.hpp"
#include "otfs/core/types.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <numbers>
#include <vector>

namespace {

[[nodiscard]] double interior_rms_error(
    std::span<const otfs::core::sample_t> actual,
    std::span<const otfs::core::sample_t> expected,
    std::size_t trim)
{
    double error = 0.0;
    double reference = 0.0;

    for (std::size_t index = trim;
         index + trim < actual.size();
         ++index) {
        error += static_cast<double>(
            std::norm(
                actual[index] -
                expected[index]));

        reference += static_cast<double>(
            std::norm(expected[index]));
    }

    return std::sqrt(
        error /
        std::max(reference, 1e-30));
}

[[nodiscard]] bool test_factor(
    std::size_t factor)
{
    constexpr std::size_t output_count = 1024U;
    const std::size_t input_count =
        output_count * factor;

    otfs::core::aligned_buffer<
        otfs::core::sample_t> input{
            input_count};

    otfs::core::aligned_buffer<
        otfs::core::sample_t> output{
            output_count};

    otfs::core::aligned_buffer<
        otfs::core::sample_t> expected{
            output_count};

    constexpr double normalized_output_frequency =
        0.05;

    for (std::size_t index = 0;
         index < input_count;
         ++index) {
        const double phase =
            2.0 *
            std::numbers::pi *
            normalized_output_frequency *
            static_cast<double>(index) /
            static_cast<double>(factor);

        input[index] = {
            static_cast<float>(
                std::cos(phase)),
            static_cast<float>(
                std::sin(phase))
        };
    }

    for (std::size_t index = 0;
         index < output_count;
         ++index) {
        const double phase =
            2.0 *
            std::numbers::pi *
            normalized_output_frequency *
            static_cast<double>(index);

        expected[index] = {
            static_cast<float>(
                std::cos(phase)),
            static_cast<float>(
                std::sin(phase))
        };
    }

    const otfs::core::integer_decimator_config config{
        .factor = factor,
        .taps = 63U,
        .cutoff_ratio = 0.90
    };

    const otfs::core::integer_decimator decimator{
        config};

    if (decimator.output_size(input_count) !=
            output_count ||
        !decimator.process_frame(
            input.span(),
            output.span())) {
        return false;
    }

    if (factor == 1U) {
        return std::equal(
            input.begin(),
            input.end(),
            output.begin());
    }

    const double error =
        interior_rms_error(
            output.span(),
            expected.span(),
            40U);

    return std::isfinite(error) &&
           error < 0.01;
}

} // namespace

int main()
{
    constexpr otfs::core::sample_rate_config pluto_rates{
        .input_sample_rate_hz = 1.92e6,
        .processing_sample_rate_hz = 0.96e6
    };

    static_assert(pluto_rates.valid());
    static_assert(
        pluto_rates.decimation_factor() == 2U);
    static_assert(
        pluto_rates.to_input_samples(2432U) ==
        4864U);
    static_assert(
        pluto_rates.to_processing_samples(4864U) ==
        2432U);

    constexpr otfs::core::sample_rate_config usrp_rates{
        .input_sample_rate_hz = 0.96e6,
        .processing_sample_rate_hz = 0.96e6
    };

    static_assert(usrp_rates.valid());
    static_assert(
        usrp_rates.decimation_factor() == 1U);

    if (!test_factor(1U) ||
        !test_factor(2U) ||
        !test_factor(4U)) {
        return EXIT_FAILURE;
    }

    constexpr std::size_t chirp_start = 37U;
    constexpr std::size_t frame_offset = 1536U;
    constexpr std::size_t processing_samples = 2432U;
    constexpr std::size_t input_samples = 4864U;

    const std::size_t capture_size =
        chirp_start +
        frame_offset +
        input_samples +
        64U;

    std::vector<otfs::core::sample_t> capture(
        capture_size,
        otfs::core::sample_t{});

    const std::size_t expected_start =
        chirp_start + frame_offset;

    for (std::size_t index = 0;
         index < input_samples;
         ++index) {
        const double phase =
            2.0 *
            std::numbers::pi *
            0.04 *
            static_cast<double>(index) /
            2.0;

        capture[expected_start + index] = {
            static_cast<float>(
                std::cos(phase)),
            static_cast<float>(
                std::sin(phase))
        };
    }

    const otfs::core::frame_extractor_config frame_config{
        .sample_rates = pluto_rates,
        .processing_frame_samples =
            processing_samples,
        .frame_offset_input_samples =
            frame_offset
    };

    const otfs::core::integer_decimator_config
        decimator_config{
            .factor = 2U,
            .taps = 63U,
            .cutoff_ratio = 0.90
        };

    const otfs::core::frame_extractor extractor{
        frame_config,
        decimator_config};

    otfs::core::aligned_buffer<
        otfs::core::sample_t> input_frame{
            input_samples};

    otfs::core::aligned_buffer<
        otfs::core::sample_t> processing_frame{
            processing_samples};

    if (!extractor.process(
            capture,
            chirp_start,
            input_frame.span(),
            processing_frame.span())) {
        return EXIT_FAILURE;
    }

    if (!std::equal(
            input_frame.begin(),
            input_frame.end(),
            capture.begin() +
                static_cast<std::ptrdiff_t>(
                    expected_start))) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
