#include "otfs/core/waveform/otfs_waveform.hpp"

#include "otfs/core/types.hpp"

#include <algorithm>
#include <stdexcept>

namespace otfs::core {

otfs_waveform::otfs_waveform(
    const waveform_config& config,
    guard_interval guard_mode)
    : config_(config),
      guard_mode_(guard_mode),
      doppler_forward_(
          config.doppler_bins,
          fft_direction::forward),
      doppler_inverse_(
          config.doppler_bins,
          fft_direction::inverse),
      row_input_(config.doppler_bins),
      row_output_(config.doppler_bins),
      unprotected_time_(config.dd_cells())
{
    if (!config_.valid()) {
        throw std::invalid_argument(
            "Invalid OTFS waveform configuration");
    }

    if (config_.zero_padding_samples > config_.delay_bins &&
        (guard_mode_ == guard_interval::cyclic_prefix ||
         guard_mode_ == guard_interval::reduced_cyclic_prefix)) {
        throw std::invalid_argument(
            "Cyclic prefix length cannot exceed M");
    }
}

const waveform_config&
otfs_waveform::config() const noexcept
{
    return config_;
}

guard_interval
otfs_waveform::guard_mode() const noexcept
{
    return guard_mode_;
}

std::size_t
otfs_waveform::payload_samples() const noexcept
{
    return config_.dd_cells();
}

std::size_t
otfs_waveform::time_domain_samples() const noexcept
{
    switch (guard_mode_) {
    case guard_interval::cyclic_prefix:
    case guard_interval::zero_padding:
        return (config_.delay_bins +
                config_.zero_padding_samples) *
               config_.doppler_bins;

    case guard_interval::reduced_cyclic_prefix:
    case guard_interval::reduced_zero_padding:
        return config_.dd_cells() +
               config_.zero_padding_samples;

    case guard_interval::none:
        return config_.dd_cells();
    }

    return 0U;
}

std::size_t
otfs_waveform::demodulation_offset() const noexcept
{
    switch (guard_mode_) {
    case guard_interval::cyclic_prefix:
    case guard_interval::reduced_cyclic_prefix:
        return config_.zero_padding_samples;

    case guard_interval::zero_padding:
    case guard_interval::reduced_zero_padding:
    case guard_interval::none:
        return 0U;
    }

    return 0U;
}

bool otfs_waveform::modulate(
    std::span<const sample_t> delay_doppler,
    std::span<sample_t> time_domain) noexcept
{
    if (delay_doppler.size() != config_.dd_cells() ||
        time_domain.size() != time_domain_samples()) {
        return false;
    }

    compute_unprotected_time_signal(delay_doppler);

    switch (guard_mode_) {
    case guard_interval::cyclic_prefix:
        serialize_full_guard(time_domain, true);
        break;

    case guard_interval::zero_padding:
        serialize_full_guard(time_domain, false);
        break;

    case guard_interval::reduced_cyclic_prefix:
        serialize_reduced_guard(time_domain, true);
        break;

    case guard_interval::reduced_zero_padding:
        serialize_reduced_guard(time_domain, false);
        break;

    case guard_interval::none:
        serialize_without_guard(time_domain);
        break;
    }

    return true;
}

bool otfs_waveform::demodulate(
    std::span<const sample_t> time_domain,
    std::span<sample_t> delay_doppler) noexcept
{
    if (time_domain.size() != time_domain_samples() ||
        delay_doppler.size() != config_.dd_cells()) {
        return false;
    }

    extract_unprotected_time_signal(time_domain);

    const float scale =
        static_cast<float>(config_.delay_bins);

    // MATLAB equivalent:
    // y = fft(Y.').' * M
    for (std::size_t delay = 0;
         delay < config_.delay_bins;
         ++delay) {
        for (std::size_t time_slot = 0;
             time_slot < config_.doppler_bins;
             ++time_slot) {
            row_input_[time_slot] =
                unprotected_time_[
                    column_major_index(
                        delay,
                        time_slot,
                        config_.delay_bins)];
        }

        doppler_forward_.execute(
            row_input_.span(),
            row_output_.span());

        for (std::size_t doppler = 0;
             doppler < config_.doppler_bins;
             ++doppler) {
            delay_doppler[
                column_major_index(
                    delay,
                    doppler,
                    config_.delay_bins)] =
                row_output_[doppler] * scale;
        }
    }

    return true;
}

void otfs_waveform::compute_unprotected_time_signal(
    std::span<const sample_t> delay_doppler) noexcept
{
    const float scale =
        1.0F /
        static_cast<float>(config_.delay_bins);

    // MATLAB equivalent:
    // y = ifft(x.').' / M
    for (std::size_t delay = 0;
         delay < config_.delay_bins;
         ++delay) {
        for (std::size_t doppler = 0;
             doppler < config_.doppler_bins;
             ++doppler) {
            row_input_[doppler] =
                delay_doppler[
                    column_major_index(
                        delay,
                        doppler,
                        config_.delay_bins)];
        }

        doppler_inverse_.execute(
            row_input_.span(),
            row_output_.span());

        for (std::size_t time_slot = 0;
             time_slot < config_.doppler_bins;
             ++time_slot) {
            unprotected_time_[
                column_major_index(
                    delay,
                    time_slot,
                    config_.delay_bins)] =
                row_output_[time_slot] * scale;
        }
    }
}

void otfs_waveform::serialize_full_guard(
    std::span<sample_t> output,
    bool cyclic) noexcept
{
    const std::size_t M = config_.delay_bins;
    const std::size_t G = config_.zero_padding_samples;
    const std::size_t slot_size = M + G;

    std::fill(output.begin(), output.end(), sample_t{});

    for (std::size_t slot = 0;
         slot < config_.doppler_bins;
         ++slot) {
        const std::size_t input_offset = slot * M;
        const std::size_t output_offset = slot * slot_size;

        if (cyclic && G > 0U) {
            std::copy_n(
                unprotected_time_.data() +
                    input_offset + M - G,
                G,
                output.begin() +
                    static_cast<std::ptrdiff_t>(
                        output_offset));
        }

        const std::size_t payload_offset =
            cyclic ? output_offset + G : output_offset;

        std::copy_n(
            unprotected_time_.data() + input_offset,
            M,
            output.begin() +
                static_cast<std::ptrdiff_t>(
                    payload_offset));
    }
}

void otfs_waveform::serialize_reduced_guard(
    std::span<sample_t> output,
    bool cyclic) noexcept
{
    const std::size_t payload = config_.dd_cells();
    const std::size_t G = config_.zero_padding_samples;

    std::fill(output.begin(), output.end(), sample_t{});

    if (cyclic && G > 0U) {
        std::copy_n(
            unprotected_time_.data() +
                payload - G,
            G,
            output.begin());

        std::copy(
            unprotected_time_.begin(),
            unprotected_time_.end(),
            output.begin() +
                static_cast<std::ptrdiff_t>(G));
    }
    else {
        std::copy(
            unprotected_time_.begin(),
            unprotected_time_.end(),
            output.begin());
    }
}

void otfs_waveform::serialize_without_guard(
    std::span<sample_t> output) noexcept
{
    std::copy(
        unprotected_time_.begin(),
        unprotected_time_.end(),
        output.begin());
}

void otfs_waveform::extract_unprotected_time_signal(
    std::span<const sample_t> input) noexcept
{
    const std::size_t M = config_.delay_bins;
    const std::size_t G = config_.zero_padding_samples;

    switch (guard_mode_) {
    case guard_interval::cyclic_prefix:
    case guard_interval::zero_padding: {
        const std::size_t slot_size = M + G;
        const std::size_t offset = demodulation_offset();

        for (std::size_t slot = 0;
             slot < config_.doppler_bins;
             ++slot) {
            std::copy_n(
                input.begin() +
                    static_cast<std::ptrdiff_t>(
                        slot * slot_size + offset),
                M,
                unprotected_time_.begin() +
                    static_cast<std::ptrdiff_t>(
                        slot * M));
        }
        break;
    }

    case guard_interval::reduced_cyclic_prefix:
    case guard_interval::reduced_zero_padding: {
        const std::size_t offset = demodulation_offset();

        std::copy_n(
            input.begin() +
                static_cast<std::ptrdiff_t>(offset),
            config_.dd_cells(),
            unprotected_time_.begin());
        break;
    }

    case guard_interval::none:
        std::copy(
            input.begin(),
            input.end(),
            unprotected_time_.begin());
        break;
    }
}

} // namespace otfs::core
