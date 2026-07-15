#pragma once

#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/types.hpp"

#include <cstddef>
#include <span>

namespace otfs::core {

struct integer_decimator_config {
    std::size_t factor{1};
    std::size_t taps{63};
    double cutoff_ratio{0.90};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return factor > 0U &&
               taps >= 3U &&
               (taps % 2U) == 1U &&
               cutoff_ratio > 0.0 &&
               cutoff_ratio < 1.0;
    }
};

class integer_decimator final {
public:
    explicit integer_decimator(
        const integer_decimator_config& config);

    integer_decimator(const integer_decimator&) = delete;
    integer_decimator& operator=(const integer_decimator&) = delete;
    integer_decimator(integer_decimator&&) noexcept = default;
    integer_decimator& operator=(integer_decimator&&) noexcept = default;

    [[nodiscard]] std::size_t factor() const noexcept;
    [[nodiscard]] std::size_t taps() const noexcept;

    [[nodiscard]] std::size_t output_size(
        std::size_t input_size) const noexcept;

    [[nodiscard]] bool process_frame(
        std::span<const sample_t> input,
        std::span<sample_t> output) const noexcept;

    [[nodiscard]] std::span<const float>
    coefficients() const noexcept;

private:
    integer_decimator_config config_{};
    aligned_buffer<float> coefficients_{};
};

} // namespace otfs::core
