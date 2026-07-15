#pragma once

#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/model/config.hpp"
#include "otfs/core/types.hpp"

#include <cstddef>
#include <span>

namespace otfs::core {

class otfs_workspace final {
public:
    explicit otfs_workspace(const receiver_config& config)
        : delay_doppler_(config.waveform.dd_cells()),
          time_frequency_(config.waveform.dd_cells()),
          time_domain_(config.waveform.base_frame_samples()),
          equalizer_input_(config.waveform.base_frame_samples()),
          equalizer_output_(config.waveform.base_frame_samples()),
          scratch_a_(config.waveform.base_frame_samples()),
          scratch_b_(config.waveform.base_frame_samples())
    {
    }

    otfs_workspace(const otfs_workspace&) = delete;
    otfs_workspace& operator=(const otfs_workspace&) = delete;
    otfs_workspace(otfs_workspace&&) noexcept = default;
    otfs_workspace& operator=(otfs_workspace&&) noexcept = default;

    [[nodiscard]] std::span<sample_t> delay_doppler() noexcept {
        return delay_doppler_.span();
    }

    [[nodiscard]] std::span<sample_t> time_frequency() noexcept {
        return time_frequency_.span();
    }

    [[nodiscard]] std::span<sample_t> time_domain() noexcept {
        return time_domain_.span();
    }

    [[nodiscard]] std::span<sample_t> equalizer_input() noexcept {
        return equalizer_input_.span();
    }

    [[nodiscard]] std::span<sample_t> equalizer_output() noexcept {
        return equalizer_output_.span();
    }

    [[nodiscard]] std::span<sample_t> scratch_a() noexcept {
        return scratch_a_.span();
    }

    [[nodiscard]] std::span<sample_t> scratch_b() noexcept {
        return scratch_b_.span();
    }

    void clear() noexcept {
        delay_doppler_.fill({});
        time_frequency_.fill({});
        time_domain_.fill({});
        equalizer_input_.fill({});
        equalizer_output_.fill({});
        scratch_a_.fill({});
        scratch_b_.fill({});
    }

private:
    aligned_buffer<sample_t> delay_doppler_;
    aligned_buffer<sample_t> time_frequency_;
    aligned_buffer<sample_t> time_domain_;
    aligned_buffer<sample_t> equalizer_input_;
    aligned_buffer<sample_t> equalizer_output_;
    aligned_buffer<sample_t> scratch_a_;
    aligned_buffer<sample_t> scratch_b_;
};

} // namespace otfs::core
