#include "otfs/core/model/config.hpp"
#include "otfs/core/pilot/pilot_masks.hpp"
#include "otfs/core/types.hpp"

#include <cstdlib>

int main()
{
    constexpr otfs::core::receiver_config config{};

    otfs::core::pilot_masks masks{config};

    // Exact expected counts for:
    // M=64, N=32, Ltau=14, K=2.
    //
    // Guard:
    // (2*Ltau+1) * (4*K+1)
    // = 29 * 9 = 261.
    //
    // Observation:
    // (Ltau+1) * (2*K+1)
    // = 15 * 5 = 75.
    //
    // Data:
    // 2048 - 261 = 1787.
    //
    // Noise:
    // 261 - 75 = 186.
    if (masks.guard_cells() != 261U ||
        masks.observation_cells() != 75U ||
        masks.search_cells() != 75U ||
        masks.data_cells() != 1787U ||
        masks.noise_cells() != 186U) {
        return EXIT_FAILURE;
    }

    if (masks.data().size() !=
            config.waveform.dd_cells() ||
        masks.guard().size() !=
            config.waveform.dd_cells() ||
        masks.observation().size() !=
            config.waveform.dd_cells() ||
        masks.search().size() !=
            config.waveform.dd_cells() ||
        masks.noise().size() !=
            config.waveform.dd_cells()) {
        return EXIT_FAILURE;
    }

    const auto pilot_index =
        otfs::core::column_major_index(
            config.pilot.delay_index,
            config.pilot.doppler_index,
            config.waveform.delay_bins);

    if (!masks.is_guard(pilot_index) ||
        !masks.is_observation(pilot_index) ||
        !masks.is_search(pilot_index) ||
        masks.is_data(pilot_index) ||
        masks.is_noise(pilot_index)) {
        return EXIT_FAILURE;
    }

    const auto first_observation =
        otfs::core::column_major_index(
            config.pilot.delay_index,
            config.pilot.doppler_index - 2U,
            config.waveform.delay_bins);

    const auto last_observation =
        otfs::core::column_major_index(
            config.pilot.delay_index +
                config.pilot.maximum_delay_bins,
            config.pilot.doppler_index + 2U,
            config.waveform.delay_bins);

    if (!masks.is_observation(first_observation) ||
        !masks.is_observation(last_observation)) {
        return EXIT_FAILURE;
    }

    const auto guard_only =
        otfs::core::column_major_index(
            config.pilot.delay_index -
                config.pilot.maximum_delay_bins,
            config.pilot.doppler_index + 4U,
            config.waveform.delay_bins);

    if (!masks.is_guard(guard_only) ||
        masks.is_observation(guard_only) ||
        !masks.is_noise(guard_only) ||
        masks.is_data(guard_only)) {
        return EXIT_FAILURE;
    }

    const auto data_index =
        otfs::core::column_major_index(
            0U,
            0U,
            config.waveform.delay_bins);

    if (!masks.is_data(data_index) ||
        masks.is_guard(data_index) ||
        masks.is_observation(data_index) ||
        masks.is_noise(data_index)) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
