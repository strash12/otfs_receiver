#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/model/config.hpp"
#include "otfs/core/pilot/pilot_observation.hpp"
#include "otfs/core/types.hpp"

#include <cmath>
#include <cstdlib>

int main()
{
    constexpr otfs::core::receiver_config config{};

    otfs::core::pilot_observation_extractor extractor{
        config};

    if (extractor.observation_size() != 75U ||
        extractor.observation_indices().size() != 75U) {
        return EXIT_FAILURE;
    }

    otfs::core::aligned_buffer<
        otfs::core::sample_t> grid{
            config.waveform.dd_cells()};

    otfs::core::aligned_buffer<
        otfs::core::sample_t> observation{
            extractor.observation_size()};

    grid.fill({});

    constexpr std::size_t path_delay_offset = 6U;
    constexpr int path_doppler_offset = -1;

    const std::size_t path_delay =
        config.pilot.delay_index +
        path_delay_offset;

    const std::size_t path_doppler =
        static_cast<std::size_t>(
            static_cast<int>(
                config.pilot.doppler_index) +
            path_doppler_offset);

    const std::size_t peak_index =
        otfs::core::column_major_index(
            path_delay,
            path_doppler,
            config.waveform.delay_bins);

    const otfs::core::sample_t expected_gain{
        0.8F,
        -0.3F
    };

    grid[peak_index] =
        expected_gain *
        config.pilot.amplitude;

    // Deterministic low-level values in noise-only guard cells.
    for (std::size_t index = 0;
         index < grid.size();
         ++index) {
        if (grid[index] == otfs::core::sample_t{}) {
            const float value =
                static_cast<float>(
                    (index % 7U) + 1U) *
                1e-4F;

            grid[index] = {
                value,
                -0.5F * value
            };
        }
    }

    otfs::core::pilot_observation_metrics metrics{};

    if (!extractor.process(
            grid.span(),
            observation.span(),
            metrics)) {
        return EXIT_FAILURE;
    }

    if (!metrics.valid ||
        metrics.peak_linear_index != peak_index ||
        metrics.peak_delay_index != path_delay ||
        metrics.peak_doppler_index != path_doppler ||
        metrics.peak_to_noise_db < 40.0 ||
        metrics.noise_variance <= 0.0) {
        return EXIT_FAILURE;
    }

    bool found_gain = false;

    const auto indices =
        extractor.observation_indices();

    for (std::size_t position = 0;
         position < indices.size();
         ++position) {
        if (indices[position] == peak_index) {
            const auto error =
                std::abs(
                    observation[position] -
                    expected_gain);

            if (error > 1e-6F) {
                return EXIT_FAILURE;
            }

            found_gain = true;
            break;
        }
    }

    if (!found_gain) {
        return EXIT_FAILURE;
    }

    otfs::core::aligned_buffer<
        otfs::core::sample_t> wrong_size{3};

    if (extractor.process(
            grid.span(),
            wrong_size.span(),
            metrics)) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
