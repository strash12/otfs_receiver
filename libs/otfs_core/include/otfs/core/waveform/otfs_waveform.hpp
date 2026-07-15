#pragma once

#include "otfs/core/fft/fft_plan.hpp"
#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/model/config.hpp"
#include "otfs/core/types.hpp"
#include "otfs/core/waveform/guard_interval.hpp"

#include <cstddef>
#include <span>

namespace otfs::core {

class otfs_waveform final {
public:
    otfs_waveform(
        const waveform_config& config,
        guard_interval guard_mode);

    otfs_waveform(const otfs_waveform&) = delete;
    otfs_waveform& operator=(const otfs_waveform&) = delete;
    otfs_waveform(otfs_waveform&&) noexcept = default;
    otfs_waveform& operator=(otfs_waveform&&) noexcept = default;

    [[nodiscard]] const waveform_config& config() const noexcept;
    [[nodiscard]] guard_interval guard_mode() const noexcept;

    [[nodiscard]] std::size_t payload_samples() const noexcept;
    [[nodiscard]] std::size_t time_domain_samples() const noexcept;
    [[nodiscard]] std::size_t demodulation_offset() const noexcept;

    [[nodiscard]] bool modulate(
        std::span<const sample_t> delay_doppler,
        std::span<sample_t> time_domain) noexcept;

    [[nodiscard]] bool demodulate(
        std::span<const sample_t> time_domain,
        std::span<sample_t> delay_doppler) noexcept;

private:
    void compute_unprotected_time_signal(
        std::span<const sample_t> delay_doppler) noexcept;

    void serialize_full_guard(
        std::span<sample_t> output,
        bool cyclic) noexcept;

    void serialize_reduced_guard(
        std::span<sample_t> output,
        bool cyclic) noexcept;

    void serialize_without_guard(
        std::span<sample_t> output) noexcept;

    void extract_unprotected_time_signal(
        std::span<const sample_t> input) noexcept;

    waveform_config config_{};
    guard_interval guard_mode_{guard_interval::zero_padding};

    fft_plan doppler_forward_;
    fft_plan doppler_inverse_;

    aligned_buffer<sample_t> row_input_;
    aligned_buffer<sample_t> row_output_;

    // M x N, column-major: one M-sample block per time slot.
    aligned_buffer<sample_t> unprotected_time_;
};

} // namespace otfs::core
