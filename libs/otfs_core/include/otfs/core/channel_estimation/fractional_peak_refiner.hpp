#pragma once

#include "otfs/core/model/channel.hpp"
#include "otfs/core/model/config.hpp"
#include "otfs/core/types.hpp"

#include <cstddef>
#include <span>

namespace otfs::core {

struct fractional_peak_refiner_config {
    double maximum_fractional_offset{0.5};
    double minimum_curvature{1e-12};
    bool refine_delay{true};
    bool refine_doppler{true};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return maximum_fractional_offset > 0.0 &&
               maximum_fractional_offset <= 0.5 &&
               minimum_curvature > 0.0;
    }
};

struct fractional_refinement_result {
    bool valid{};
    std::size_t refined_paths{};
    std::size_t delay_refinements{};
    std::size_t doppler_refinements{};
};

class fractional_peak_refiner final {
public:
    fractional_peak_refiner(
        const receiver_config& receiver_config,
        const fractional_peak_refiner_config& refiner_config);

    [[nodiscard]] fractional_refinement_result refine(
        std::span<const sample_t> normalized_delay_doppler,
        std::span<channel_path> paths) const noexcept;

private:
    [[nodiscard]] double power_at(
        std::span<const sample_t> grid,
        std::size_t delay,
        std::size_t doppler) const noexcept;

    [[nodiscard]] double parabolic_offset(
        double left,
        double center,
        double right) const noexcept;

    [[nodiscard]] std::size_t wrap_doppler(
        std::ptrdiff_t doppler) const noexcept;

    receiver_config receiver_config_{};
    fractional_peak_refiner_config refiner_config_{};
};

} // namespace otfs::core
