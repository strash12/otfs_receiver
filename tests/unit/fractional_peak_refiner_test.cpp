#include "otfs/core/channel_estimation/fractional_peak_refiner.hpp"
#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/model/channel.hpp"
#include "otfs/core/model/config.hpp"
#include "otfs/core/types.hpp"

#include <cmath>
#include <cstdlib>

namespace {

void add_quadratic_peak(
    std::span<otfs::core::sample_t> grid,
    const otfs::core::receiver_config& config,
    double relative_delay,
    double relative_doppler,
    float amplitude)
{
    const double absolute_delay =
        static_cast<double>(
            config.pilot.delay_index) +
        relative_delay;

    double absolute_doppler =
        static_cast<double>(
            config.pilot.doppler_index) +
        relative_doppler;

    const double N =
        static_cast<double>(
            config.waveform.doppler_bins);

    while (absolute_doppler < 0.0) {
        absolute_doppler += N;
    }

    while (absolute_doppler >= N) {
        absolute_doppler -= N;
    }

    const auto delay_center =
        static_cast<std::ptrdiff_t>(
            std::llround(absolute_delay));

    const auto doppler_center =
        static_cast<std::ptrdiff_t>(
            std::llround(absolute_doppler));

    for (std::ptrdiff_t delay_offset = -1;
         delay_offset <= 1;
         ++delay_offset) {
        const auto delay =
            delay_center + delay_offset;

        if (delay < 0 ||
            delay >= static_cast<std::ptrdiff_t>(
                config.waveform.delay_bins)) {
            continue;
        }

        for (std::ptrdiff_t doppler_offset = -1;
             doppler_offset <= 1;
             ++doppler_offset) {
            auto doppler =
                doppler_center + doppler_offset;

            const auto doppler_bins =
                static_cast<std::ptrdiff_t>(
                    config.waveform.doppler_bins);

            doppler %= doppler_bins;

            if (doppler < 0) {
                doppler += doppler_bins;
            }

            const double delay_error =
                static_cast<double>(delay) -
                absolute_delay;

            double doppler_error =
                static_cast<double>(doppler) -
                absolute_doppler;

            if (doppler_error > N / 2.0) {
                doppler_error -= N;
            }
            else if (doppler_error < -N / 2.0) {
                doppler_error += N;
            }

            const double power =
                std::max(
                    0.0,
                    1.0 -
                    0.35 * delay_error * delay_error -
                    0.30 * doppler_error * doppler_error);

            const float magnitude =
                amplitude *
                static_cast<float>(
                    std::sqrt(power));

            const auto index =
                otfs::core::column_major_index(
                    static_cast<std::size_t>(delay),
                    static_cast<std::size_t>(doppler),
                    config.waveform.delay_bins);

            grid[index] +=
                otfs::core::sample_t{
                    magnitude,
                    0.0F
                };
        }
    }
}

} // namespace

int main()
{
    constexpr otfs::core::receiver_config config{};

    otfs::core::aligned_buffer<
        otfs::core::sample_t> grid{
            config.waveform.dd_cells()};

    grid.fill({});

    constexpr double true_delay = 6.30;
    constexpr double true_doppler = -1.35;

    add_quadratic_peak(
        grid.span(),
        config,
        true_delay,
        true_doppler,
        1.0F);

    otfs::core::aligned_buffer<
        otfs::core::channel_path> paths{1U};

    paths[0] = {
        .gain = {1.0F, 0.0F},
        .delay_samples = 6.0,
        .doppler_bins = -1.0
    };

    const otfs::core::fractional_peak_refiner_config
        refiner_config{
            .maximum_fractional_offset = 0.5,
            .minimum_curvature = 1e-12,
            .refine_delay = true,
            .refine_doppler = true
        };

    const otfs::core::fractional_peak_refiner refiner{
        config,
        refiner_config};

    const auto result =
        refiner.refine(
            grid.span(),
            paths.span());

    if (!result.valid ||
        result.refined_paths != 1U ||
        result.delay_refinements != 1U ||
        result.doppler_refinements != 1U) {
        return EXIT_FAILURE;
    }

    if (std::abs(
            paths[0].delay_samples -
            true_delay) > 1e-6 ||
        std::abs(
            paths[0].doppler_bins -
            true_doppler) > 1e-6) {
        return EXIT_FAILURE;
    }

    // Проверка циклической границы по Доплеру.
    grid.fill({});

    constexpr double wrapped_delay = 3.20;
    constexpr double wrapped_doppler = 15.40;

    add_quadratic_peak(
        grid.span(),
        config,
        wrapped_delay,
        wrapped_doppler,
        0.8F);

    paths[0] = {
        .gain = {0.8F, 0.0F},
        .delay_samples = 3.0,
        .doppler_bins = 15.0
    };

    const auto wrapped_result =
        refiner.refine(
            grid.span(),
            paths.span());

    if (!wrapped_result.valid ||
        std::abs(
            paths[0].delay_samples -
            wrapped_delay) > 1e-6 ||
        std::abs(
            paths[0].doppler_bins -
            wrapped_doppler) > 1e-6) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
