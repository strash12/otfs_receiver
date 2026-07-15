#include "otfs/core/pilot/pilot_masks.hpp"

#include "otfs/core/types.hpp"

#include <algorithm>
#include <stdexcept>

namespace otfs::core {

namespace {

[[nodiscard]] std::size_t wrap_doppler(
    int index,
    std::size_t doppler_bins) noexcept
{
    const int size = static_cast<int>(doppler_bins);
    int wrapped = index % size;

    if (wrapped < 0) {
        wrapped += size;
    }

    return static_cast<std::size_t>(wrapped);
}

} // namespace

pilot_masks::pilot_masks(const receiver_config& config)
    : data_(config.waveform.dd_cells()),
      guard_(config.waveform.dd_cells()),
      observation_(config.waveform.dd_cells()),
      search_(config.waveform.dd_cells()),
      noise_(config.waveform.dd_cells())
{
    if (!config.valid()) {
        throw std::invalid_argument(
            "Invalid receiver configuration for pilot masks");
    }

    data_.fill(1U);
    guard_.fill(0U);
    observation_.fill(0U);
    search_.fill(0U);
    noise_.fill(0U);

    const auto& waveform = config.waveform;
    const auto& pilot = config.pilot;

    const int pilot_delay =
        static_cast<int>(pilot.delay_index);

    const int pilot_doppler =
        static_cast<int>(pilot.doppler_index);

    const int maximum_delay =
        static_cast<int>(pilot.maximum_delay_bins);

    const int observation_radius =
        static_cast<int>(
            pilot.observation_doppler_radius);

    // Exact MATLAB guard region:
    // delay offsets   = -Ltau : +Ltau
    // Doppler offsets = -2*K : +2*K
    for (int delay_offset = -maximum_delay;
         delay_offset <= maximum_delay;
         ++delay_offset) {
        const int delay = pilot_delay + delay_offset;

        if (delay < 0 ||
            delay >= static_cast<int>(
                waveform.delay_bins)) {
            throw std::invalid_argument(
                "Pilot guard exceeds delay grid");
        }

        for (int doppler_offset =
                 -2 * observation_radius;
             doppler_offset <=
                 2 * observation_radius;
             ++doppler_offset) {
            const auto doppler =
                wrap_doppler(
                    pilot_doppler + doppler_offset,
                    waveform.doppler_bins);

            const auto index =
                column_major_index(
                    static_cast<std::size_t>(delay),
                    doppler,
                    waveform.delay_bins);

            guard_[index] = 1U;
            data_[index] = 0U;
        }
    }

    // Exact MATLAB observation/search region:
    // delay offsets   = 0 : Ltau
    // Doppler offsets = -K : +K
    for (int delay_offset = 0;
         delay_offset <= maximum_delay;
         ++delay_offset) {
        const auto delay =
            static_cast<std::size_t>(
                pilot_delay + delay_offset);

        for (int doppler_offset =
                 -observation_radius;
             doppler_offset <=
                 observation_radius;
             ++doppler_offset) {
            const auto doppler =
                wrap_doppler(
                    pilot_doppler + doppler_offset,
                    waveform.doppler_bins);

            const auto index =
                column_major_index(
                    delay,
                    doppler,
                    waveform.delay_bins);

            observation_[index] = 1U;
            search_[index] = 1U;
        }
    }

    for (std::size_t index = 0;
         index < waveform.dd_cells();
         ++index) {
        noise_[index] =
            static_cast<std::uint8_t>(
                guard_[index] != 0U &&
                observation_[index] == 0U);
    }

    data_cells_ = count_nonzero(data_.span());
    guard_cells_ = count_nonzero(guard_.span());
    observation_cells_ =
        count_nonzero(observation_.span());
    search_cells_ = count_nonzero(search_.span());
    noise_cells_ = count_nonzero(noise_.span());
}

std::span<const std::uint8_t>
pilot_masks::data() const noexcept
{
    return data_.span();
}

std::span<const std::uint8_t>
pilot_masks::guard() const noexcept
{
    return guard_.span();
}

std::span<const std::uint8_t>
pilot_masks::observation() const noexcept
{
    return observation_.span();
}

std::span<const std::uint8_t>
pilot_masks::search() const noexcept
{
    return search_.span();
}

std::span<const std::uint8_t>
pilot_masks::noise() const noexcept
{
    return noise_.span();
}

std::size_t pilot_masks::data_cells() const noexcept
{
    return data_cells_;
}

std::size_t pilot_masks::guard_cells() const noexcept
{
    return guard_cells_;
}

std::size_t
pilot_masks::observation_cells() const noexcept
{
    return observation_cells_;
}

std::size_t pilot_masks::search_cells() const noexcept
{
    return search_cells_;
}

std::size_t pilot_masks::noise_cells() const noexcept
{
    return noise_cells_;
}

bool pilot_masks::is_data(
    std::size_t index) const noexcept
{
    return index < data_.size() &&
           data_[index] != 0U;
}

bool pilot_masks::is_guard(
    std::size_t index) const noexcept
{
    return index < guard_.size() &&
           guard_[index] != 0U;
}

bool pilot_masks::is_observation(
    std::size_t index) const noexcept
{
    return index < observation_.size() &&
           observation_[index] != 0U;
}

bool pilot_masks::is_search(
    std::size_t index) const noexcept
{
    return index < search_.size() &&
           search_[index] != 0U;
}

bool pilot_masks::is_noise(
    std::size_t index) const noexcept
{
    return index < noise_.size() &&
           noise_[index] != 0U;
}

std::size_t pilot_masks::count_nonzero(
    std::span<const std::uint8_t> mask) noexcept
{
    return static_cast<std::size_t>(
        std::count_if(
            mask.begin(),
            mask.end(),
            [](std::uint8_t value) {
                return value != 0U;
            }));
}

} // namespace otfs::core
