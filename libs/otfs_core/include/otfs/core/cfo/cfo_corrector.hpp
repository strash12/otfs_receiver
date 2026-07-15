#pragma once

#include "otfs/core/types.hpp"

#include <span>

namespace otfs::core {

class cfo_corrector final {
public:
    explicit cfo_corrector(double sample_rate_hz);

    void reset(double initial_phase_radians = 0.0) noexcept;

    [[nodiscard]] bool process(
        std::span<const sample_t> input,
        std::span<sample_t> output,
        double frequency_hz) noexcept;

    [[nodiscard]] double phase_radians() const noexcept;

private:
    double sample_rate_hz_{};
    sample_t current_phasor_{1.0F, 0.0F};
    double phase_radians_{};
};

} // namespace otfs::core
