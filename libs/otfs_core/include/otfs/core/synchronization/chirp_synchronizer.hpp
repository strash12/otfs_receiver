#pragma once

#include "otfs/core/fft/fft_plan.hpp"
#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/types.hpp"

#include <cstddef>
#include <span>

namespace otfs::core {

struct chirp_sync_config {
    std::size_t payload_length{};
    float minimum_peak_db{10.0F};
    float minimum_rho{0.25F};

    [[nodiscard]] constexpr bool valid(
        std::size_t chirp_length,
        std::size_t maximum_capture_length) const noexcept
    {
        return chirp_length >= 2U &&
               payload_length >= chirp_length &&
               maximum_capture_length >= payload_length &&
               minimum_rho >= 0.0F &&
               minimum_rho <= 1.0F;
    }
};

struct chirp_sync_result {
    bool found{};
    std::size_t chirp_start{};
    std::size_t matched_filter_peak{};
    float peak_db{};
    float rho{};
    float peak_value{};
};

class chirp_synchronizer final {
public:
    chirp_synchronizer(
        std::span<const sample_t> chirp_reference,
        const chirp_sync_config& config,
        std::size_t maximum_capture_length);

    chirp_synchronizer(const chirp_synchronizer&) = delete;
    chirp_synchronizer& operator=(const chirp_synchronizer&) = delete;
    chirp_synchronizer(chirp_synchronizer&&) noexcept = default;
    chirp_synchronizer& operator=(chirp_synchronizer&&) noexcept = default;

    [[nodiscard]] chirp_sync_result process(
        std::span<const sample_t> capture) noexcept;

    [[nodiscard]] std::span<const float>
    matched_filter_magnitude() const noexcept;

    [[nodiscard]] std::size_t chirp_length() const noexcept;
    [[nodiscard]] std::size_t fft_length() const noexcept;
    [[nodiscard]] std::size_t maximum_capture_length() const noexcept;

private:
    [[nodiscard]] static std::size_t next_power_of_two(
        std::size_t value) noexcept;

    [[nodiscard]] float compute_background_median(
        std::size_t capture_length,
        std::size_t peak_index,
        std::size_t exclusion) noexcept;

    chirp_sync_config config_{};
    std::size_t maximum_capture_length_{};
    std::size_t chirp_length_{};
    std::size_t fft_length_{};

    fft_plan forward_;
    fft_plan inverse_;

    aligned_buffer<sample_t> chirp_reference_;
    aligned_buffer<sample_t> kernel_spectrum_;
    aligned_buffer<sample_t> fft_input_;
    aligned_buffer<sample_t> capture_spectrum_;
    aligned_buffer<sample_t> product_spectrum_;
    aligned_buffer<sample_t> convolution_;

    aligned_buffer<float> matched_filter_magnitude_;
    aligned_buffer<float> median_workspace_;
};

} // namespace otfs::core
