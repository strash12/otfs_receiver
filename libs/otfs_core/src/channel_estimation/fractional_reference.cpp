#include "otfs/core/channel_estimation/fractional_reference.hpp"

#include <cmath>
#include <complex>
#include <numbers>
#include <stdexcept>

namespace otfs::core {

fractional_reference_generator::fractional_reference_generator(
    std::span<const sample_t> pilot_time,
    const receiver_config& receiver_config,
    guard_interval guard_mode)
    : receiver_config_(receiver_config),
      guard_mode_(guard_mode),
      forward_(pilot_time.size(), fft_direction::forward),
      inverse_(pilot_time.size(), fft_direction::inverse),
      waveform_(receiver_config.waveform, guard_mode),
      observation_extractor_(receiver_config),
      pilot_spectrum_(pilot_time.size()),
      delayed_spectrum_(pilot_time.size()),
      delayed_time_(pilot_time.size()),
      reference_time_(pilot_time.size()),
      reference_dd_(receiver_config.waveform.dd_cells())
{
    if (!receiver_config_.valid()) {
        throw std::invalid_argument(
            "Invalid receiver configuration");
    }

    if (pilot_time.size() != waveform_.time_domain_samples()) {
        throw std::invalid_argument(
            "Pilot time length does not match waveform");
    }

    forward_.execute(
        pilot_time,
        pilot_spectrum_.span());
}

bool fractional_reference_generator::make_observation_reference(
    double delay_samples,
    double doppler_bins,
    std::span<sample_t> observation_reference) noexcept
{
    if (observation_reference.size() !=
            observation_extractor_.observation_size() ||
        !std::isfinite(delay_samples) ||
        !std::isfinite(doppler_bins)) {
        return false;
    }

    const std::size_t length =
        pilot_spectrum_.size();

    for (std::size_t index = 0;
         index < length;
         ++index) {
        std::ptrdiff_t signed_index =
            static_cast<std::ptrdiff_t>(index);

        if (index >= length / 2U) {
            signed_index -=
                static_cast<std::ptrdiff_t>(length);
        }

        const double phase =
            -2.0 *
            std::numbers::pi *
            static_cast<double>(signed_index) *
            delay_samples /
            static_cast<double>(length);

        const sample_t delay_phasor{
            static_cast<float>(std::cos(phase)),
            static_cast<float>(std::sin(phase))
        };

        delayed_spectrum_[index] =
            pilot_spectrum_[index] *
            delay_phasor;
    }

    inverse_.execute(
        delayed_spectrum_.span(),
        delayed_time_.span());

    for (std::size_t index = 0;
         index < length;
         ++index) {
        const double phase =
            2.0 *
            std::numbers::pi *
            static_cast<double>(index) *
            doppler_bins /
            static_cast<double>(length);

        const sample_t doppler_phasor{
            static_cast<float>(std::cos(phase)),
            static_cast<float>(std::sin(phase))
        };

        reference_time_[index] =
            delayed_time_[index] *
            doppler_phasor;
    }

    if (!waveform_.demodulate(
            reference_time_.span(),
            reference_dd_.span())) {
        return false;
    }

    const auto indices =
        observation_extractor_.observation_indices();

    for (std::size_t position = 0;
         position < indices.size();
         ++position) {
        observation_reference[position] =
            reference_dd_[indices[position]];
    }

    return true;
}

std::size_t
fractional_reference_generator::frame_length() const noexcept
{
    return pilot_spectrum_.size();
}

std::size_t
fractional_reference_generator::observation_size() const noexcept
{
    return observation_extractor_.observation_size();
}

} // namespace otfs::core
