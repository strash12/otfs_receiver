#include "otfs/core/frontend/dc_remover.hpp"

#include <algorithm>
#include <stdexcept>

namespace otfs::core {

dc_remover::dc_remover(
    const dc_removal_config& config)
    : config_(config)
{
    if (!config_.valid()) {
        throw std::invalid_argument(
            "Invalid DC removal configuration");
    }
}

void dc_remover::reset() noexcept
{
    estimate_ = {};
}

void dc_remover::process(
    std::span<const sample_t> input,
    std::span<sample_t> output) noexcept
{
    if (input.size() != output.size()) {
        return;
    }

    if (!config_.enabled) {
        std::copy(
            input.begin(),
            input.end(),
            output.begin());
        return;
    }

    const float alpha = config_.forgetting_factor;
    const float one_minus_alpha = 1.0F - alpha;

    for (std::size_t index = 0;
         index < input.size();
         ++index) {
        estimate_ =
            alpha * estimate_ +
            one_minus_alpha * input[index];

        output[index] =
            input[index] - estimate_;
    }
}

sample_t dc_remover::estimate() const noexcept
{
    return estimate_;
}

} // namespace otfs::core
