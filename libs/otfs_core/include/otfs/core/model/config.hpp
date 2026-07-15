#pragma once

#include <cstddef>

namespace otfs::core {

struct waveform_config {
    std::size_t delay_bins{64};
    std::size_t doppler_bins{32};
    std::size_t zero_padding_samples{12};
    double subcarrier_spacing_hz{15'000.0};
    std::size_t oversampling_factor{2};

    [[nodiscard]] constexpr std::size_t dd_cells() const noexcept {
        return delay_bins * doppler_bins;
    }

    [[nodiscard]] constexpr std::size_t base_frame_samples() const noexcept {
        return (delay_bins + zero_padding_samples) * doppler_bins;
    }

    [[nodiscard]] constexpr double base_sample_rate_hz() const noexcept {
        return static_cast<double>(delay_bins) *
               subcarrier_spacing_hz;
    }

    [[nodiscard]] constexpr double radio_sample_rate_hz() const noexcept {
        return static_cast<double>(oversampling_factor) *
               base_sample_rate_hz();
    }

    [[nodiscard]] constexpr bool valid() const noexcept {
        return delay_bins > 0 &&
               doppler_bins > 0 &&
               zero_padding_samples < delay_bins &&
               subcarrier_spacing_hz > 0.0 &&
               oversampling_factor > 0;
    }
};

struct pilot_config {
    std::size_t delay_index{32};
    std::size_t doppler_index{16};
    float amplitude{15.0F};
    std::size_t maximum_delay_bins{14};
    std::size_t observation_doppler_radius{2};

    [[nodiscard]] constexpr bool valid(
        const waveform_config& waveform) const noexcept
    {
        return delay_index < waveform.delay_bins &&
               doppler_index < waveform.doppler_bins &&
               amplitude > 0.0F &&
               delay_index + maximum_delay_bins <
                   waveform.delay_bins;
    }
};

struct synchronization_config {
    float minimum_peak_db{8.0F};
    float minimum_normalized_correlation{0.45F};
    std::size_t chirp_length{1024};
    std::size_t guard_samples_radio{512};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return minimum_normalized_correlation >= 0.0F &&
               minimum_normalized_correlation <= 1.0F &&
               chirp_length > 1;
    }
};

struct equalizer_config {
    std::size_t maximum_iterations{24};
    double tolerance{1e-4};
    double regularization_floor{1e-8};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return maximum_iterations > 0 &&
               tolerance > 0.0 &&
               regularization_floor >= 0.0;
    }
};

struct receiver_config {
    waveform_config waveform{};
    pilot_config pilot{};
    synchronization_config synchronization{};
    equalizer_config equalizer{};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return waveform.valid() &&
               pilot.valid(waveform) &&
               synchronization.valid() &&
               equalizer.valid();
    }
};

} // namespace otfs::core
