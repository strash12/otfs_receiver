#include "otfs/core/frontend/power_meter.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace otfs::core {

power_meter::power_meter(
    const power_meter_config& config)
    : config_(config)
{
    if (!config_.valid()) {
        throw std::invalid_argument(
            "Invalid power meter configuration");
    }
}

void power_meter::reset() noexcept
{
}

double power_meter::rms(
    std::span<const sample_t> samples) const noexcept
{
    if (samples.empty()) {
        return 0.0;
    }

    const std::size_t count =
        std::min(
            samples.size(),
            config_.averaging_window);

    const std::size_t start =
        samples.size() - count;

    double accumulated_power = 0.0;

    for (std::size_t index = start;
         index < samples.size();
         ++index) {
        accumulated_power +=
            static_cast<double>(
                std::norm(samples[index]));
    }

    return std::sqrt(
        accumulated_power /
        static_cast<double>(count));
}

double power_meter::power_dbfs(
    std::span<const sample_t> samples) const noexcept
{
    const double value = rms(samples);

    if (value <=
        std::numeric_limits<double>::min()) {
        return -300.0;
    }

    return 20.0 * std::log10(value);
}

} // namespace otfs::core
