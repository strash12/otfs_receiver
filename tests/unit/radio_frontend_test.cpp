#include "otfs/core/frontend/frontend_config.hpp"
#include "otfs/core/frontend/frontend_metrics.hpp"
#include "otfs/core/frontend/radio_frontend.hpp"
#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/types.hpp"

#include <cmath>
#include <cstdlib>
#include <numbers>

namespace {

[[nodiscard]] double mean_real(
    std::span<const otfs::core::sample_t> samples)
{
    double sum = 0.0;

    for (const auto value : samples) {
        sum += static_cast<double>(value.real());
    }

    return samples.empty()
        ? 0.0
        : sum / static_cast<double>(samples.size());
}

[[nodiscard]] double mean_imag(
    std::span<const otfs::core::sample_t> samples)
{
    double sum = 0.0;

    for (const auto value : samples) {
        sum += static_cast<double>(value.imag());
    }

    return samples.empty()
        ? 0.0
        : sum / static_cast<double>(samples.size());
}

} // namespace

int main()
{
    constexpr std::size_t count = 4096U;

    otfs::core::aligned_buffer<
        otfs::core::sample_t> input{count};

    otfs::core::aligned_buffer<
        otfs::core::sample_t> output{count};

    for (std::size_t index = 0;
         index < count;
         ++index) {
        const double phase =
            2.0 * std::numbers::pi *
            37.0 *
            static_cast<double>(index) /
            static_cast<double>(count);

        input[index] = {
            static_cast<float>(
                0.35 * std::cos(phase) + 0.08),
            static_cast<float>(
                0.35 * std::sin(phase) - 0.04)
        };
    }

    otfs::core::radio_frontend_config config{};
    config.dc_removal.enabled = true;
    config.dc_removal.forgetting_factor = 0.98F;
    config.power_meter.averaging_window = count;
    config.agc.enabled = true;
    config.agc.target_rms = 0.25F;
    config.agc.attack = 1.0F;
    config.agc.release = 1.0F;

    otfs::core::radio_frontend frontend{
        config,
        count};

    otfs::core::frontend_metrics metrics{};

    if (!frontend.process(
            input.span(),
            output.span(),
            metrics)) {
        return EXIT_FAILURE;
    }

    const double output_mean_real =
        mean_real(output.span());

    const double output_mean_imag =
        mean_imag(output.span());

    if (std::abs(output_mean_real) > 5e-3 ||
        std::abs(output_mean_imag) > 5e-3) {
        return EXIT_FAILURE;
    }

    if (!std::isfinite(metrics.input_rms) ||
        !std::isfinite(metrics.output_rms) ||
        !std::isfinite(metrics.input_power_dbfs) ||
        !std::isfinite(metrics.output_power_dbfs) ||
        !std::isfinite(metrics.applied_gain)) {
        return EXIT_FAILURE;
    }

    if (std::abs(metrics.output_rms - 0.25) > 0.02) {
        return EXIT_FAILURE;
    }

    if (!metrics.signal_present ||
        metrics.applied_gain <= 0.0F) {
        return EXIT_FAILURE;
    }

    otfs::core::radio_frontend_config bypass_config{};
    bypass_config.dc_removal.enabled = false;
    bypass_config.agc.enabled = false;
    bypass_config.power_meter.averaging_window = count;

    otfs::core::radio_frontend bypass{
        bypass_config,
        count};

    if (!bypass.process(
            input.span(),
            output.span(),
            metrics)) {
        return EXIT_FAILURE;
    }

    for (std::size_t index = 0;
         index < count;
         ++index) {
        if (output[index] != input[index]) {
            return EXIT_FAILURE;
        }
    }

    otfs::core::aligned_buffer<
        otfs::core::sample_t> wrong_size{7};

    if (frontend.process(
            input.span(),
            wrong_size.span(),
            metrics)) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
