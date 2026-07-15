#include "otfs/core/frontend/radio_frontend.hpp"

#include <stdexcept>

namespace otfs::core {

radio_frontend::radio_frontend(
    const radio_frontend_config& config,
    std::size_t maximum_block_size)
    : config_(config),
      maximum_block_size_(maximum_block_size),
      dc_remover_(config.dc_removal),
      power_meter_(config.power_meter),
      agc_(config.agc),
      dc_removed_(maximum_block_size)
{
    if (!config_.valid()) {
        throw std::invalid_argument(
            "Invalid radio front-end configuration");
    }

    if (maximum_block_size_ == 0U) {
        throw std::invalid_argument(
            "Maximum block size must be positive");
    }
}

void radio_frontend::reset() noexcept
{
    dc_remover_.reset();
    power_meter_.reset();
    agc_.reset();
}

bool radio_frontend::process(
    std::span<const sample_t> input,
    std::span<sample_t> output,
    frontend_metrics& metrics) noexcept
{
    if (input.size() != output.size() ||
        input.size() > maximum_block_size_) {
        return false;
    }

    const auto dc_view =
        dc_removed_.span().first(input.size());

    metrics.input_rms =
        power_meter_.rms(input);

    metrics.input_power_dbfs =
        power_meter_.power_dbfs(input);

    dc_remover_.process(
        input,
        dc_view);

    metrics.estimated_dc =
        dc_remover_.estimate();

    const double rms_after_dc =
        power_meter_.rms(dc_view);

    agc_.process(
        dc_view,
        output,
        rms_after_dc);

    metrics.output_rms =
        power_meter_.rms(output);

    metrics.output_power_dbfs =
        power_meter_.power_dbfs(output);

    metrics.applied_gain =
        agc_.gain();

    metrics.signal_present =
        metrics.output_rms > 1e-4;

    return true;
}

std::size_t
radio_frontend::maximum_block_size() const noexcept
{
    return maximum_block_size_;
}

} // namespace otfs::core
