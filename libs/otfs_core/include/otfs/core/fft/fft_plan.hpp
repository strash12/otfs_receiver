#pragma once

#include "otfs/core/fft/fft_direction.hpp"
#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/types.hpp"

#include <cstddef>
#include <span>

struct fftwf_plan_s;

namespace otfs::core {

class fft_plan final {
public:
    fft_plan(
        std::size_t length,
        fft_direction direction);

    ~fft_plan() noexcept;

    fft_plan(const fft_plan&) = delete;
    fft_plan& operator=(const fft_plan&) = delete;

    fft_plan(fft_plan&& other) noexcept;
    fft_plan& operator=(fft_plan&& other) noexcept;

    [[nodiscard]] std::size_t length() const noexcept;
    [[nodiscard]] fft_direction direction() const noexcept;

    void execute(
        std::span<const sample_t> input,
        std::span<sample_t> output) noexcept;

private:
    void release() noexcept;

    std::size_t length_{};
    fft_direction direction_{fft_direction::forward};
    aligned_buffer<sample_t> input_buffer_{};
    aligned_buffer<sample_t> output_buffer_{};
    fftwf_plan_s* plan_{};
};

} // namespace otfs::core
