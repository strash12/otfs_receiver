#pragma once

#include "otfs/core/fft/fft_plan.hpp"
#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/model/config.hpp"
#include "otfs/core/pilot/pilot_observation.hpp"
#include "otfs/core/types.hpp"
#include "otfs/core/waveform/guard_interval.hpp"
#include "otfs/core/waveform/otfs_waveform.hpp"

#include <cstddef>
#include <span>

namespace otfs::core {

class fractional_reference_generator final {
public:
    fractional_reference_generator(
        std::span<const sample_t> pilot_time,
        const receiver_config& receiver_config,
        guard_interval guard_mode);

    fractional_reference_generator(
        const fractional_reference_generator&) = delete;
    fractional_reference_generator& operator=(
        const fractional_reference_generator&) = delete;
    fractional_reference_generator(
        fractional_reference_generator&&) noexcept = default;
    fractional_reference_generator& operator=(
        fractional_reference_generator&&) noexcept = default;

    [[nodiscard]] bool make_observation_reference(
        double delay_samples,
        double doppler_bins,
        std::span<sample_t> observation_reference) noexcept;

    [[nodiscard]] std::size_t frame_length() const noexcept;
    [[nodiscard]] std::size_t observation_size() const noexcept;

private:
    receiver_config receiver_config_{};
    guard_interval guard_mode_{guard_interval::zero_padding};

    fft_plan forward_;
    fft_plan inverse_;
    otfs_waveform waveform_;
    pilot_observation_extractor observation_extractor_;

    aligned_buffer<sample_t> pilot_spectrum_;
    aligned_buffer<sample_t> delayed_spectrum_;
    aligned_buffer<sample_t> delayed_time_;
    aligned_buffer<sample_t> reference_time_;
    aligned_buffer<sample_t> reference_dd_;
};

} // namespace otfs::core
