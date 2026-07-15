#include "otfs/core/cfo/cfo_corrector.hpp"

#include <cmath>
#include <numbers>
#include <stdexcept>

namespace otfs::core {

cfo_corrector::cfo_corrector(double sample_rate_hz)
    : sample_rate_hz_(sample_rate_hz)
{
    if (sample_rate_hz_ <= 0.0) {
        throw std::invalid_argument(
            "Sample rate must be positive");
    }
}

void cfo_corrector::reset(
    double initial_phase_radians) noexcept
{
    phase_radians_ = initial_phase_radians;

    current_phasor_ = {
        static_cast<float>(
            std::cos(-phase_radians_)),
        static_cast<float>(
            std::sin(-phase_radians_))
    };
}

bool cfo_corrector::process(
    std::span<const sample_t> input,
    std::span<sample_t> output,
    double frequency_hz) noexcept
{
    if (input.size() != output.size() ||
        !std::isfinite(frequency_hz)) {
        return false;
    }

    const double phase_step =
        -2.0 *
        std::numbers::pi *
        frequency_hz /
        sample_rate_hz_;

    const sample_t step{
        static_cast<float>(
            std::cos(phase_step)),
        static_cast<float>(
            std::sin(phase_step))
    };

    sample_t phasor = current_phasor_;

    for (std::size_t index = 0;
         index < input.size();
         ++index) {
        output[index] = input[index] * phasor;
        phasor *= step;

        if ((index & 1023U) == 1023U) {
            const float magnitude = std::abs(phasor);

            if (magnitude > 0.0F) {
                phasor /= magnitude;
            }
        }
    }

    current_phasor_ = phasor;

    phase_radians_ +=
        2.0 *
        std::numbers::pi *
        frequency_hz *
        static_cast<double>(input.size()) /
        sample_rate_hz_;

    phase_radians_ =
        std::remainder(
            phase_radians_,
            2.0 * std::numbers::pi);

    return true;
}

double cfo_corrector::phase_radians() const noexcept
{
    return phase_radians_;
}

} // namespace otfs::core
