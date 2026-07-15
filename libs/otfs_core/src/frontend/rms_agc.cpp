#include "otfs/core/frontend/rms_agc.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace otfs::core {

rms_agc::rms_agc(const agc_config& config)
    : config_(config)
{
    if (!config_.valid()) {
        throw std::invalid_argument(
            "Invalid AGC configuration");
    }
}

void rms_agc::reset() noexcept
{
    gain_ = 1.0F;
}

void rms_agc::process(
    std::span<const sample_t> input,
    std::span<sample_t> output,
    double measured_rms) noexcept
{
    if (input.size() != output.size()) {
        return;
    }

    if (!config_.enabled) {
        std::copy(
            input.begin(),
            input.end(),
            output.begin());
        gain_ = 1.0F;
        return;
    }

    const float safe_rms =
        static_cast<float>(
            std::max(
                measured_rms,
                static_cast<double>(config_.epsilon)));

    const float desired_gain =
        std::clamp(
            config_.target_rms / safe_rms,
            config_.minimum_gain,
            config_.maximum_gain);

    const float coefficient =
        desired_gain < gain_
            ? config_.attack
            : config_.release;

    gain_ =
        coefficient * desired_gain +
        (1.0F - coefficient) * gain_;

    gain_ = std::clamp(
        gain_,
        config_.minimum_gain,
        config_.maximum_gain);

    for (std::size_t index = 0;
         index < input.size();
         ++index) {
        output[index] =
            gain_ * input[index];
    }
}

float rms_agc::gain() const noexcept
{
    return gain_;
}

} // namespace otfs::core
