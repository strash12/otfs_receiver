#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/model/config.hpp"
#include "otfs/core/types.hpp"
#include "otfs/core/waveform/guard_interval.hpp"
#include "otfs/core/waveform/otfs_waveform.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <numbers>
#include <span>

namespace {

[[nodiscard]] double relative_error(
    std::span<const otfs::core::sample_t> lhs,
    std::span<const otfs::core::sample_t> rhs)
{
    double numerator = 0.0;
    double denominator = 0.0;

    for (std::size_t index = 0;
         index < lhs.size();
         ++index) {
        numerator += static_cast<double>(
            std::norm(lhs[index] - rhs[index]));

        denominator += static_cast<double>(
            std::norm(rhs[index]));
    }

    return std::sqrt(
        numerator /
        std::max(denominator, 1e-30));
}

[[nodiscard]] bool run_round_trip(
    otfs::core::guard_interval mode,
    const otfs::core::waveform_config& config,
    std::span<const otfs::core::sample_t> input)
{
    otfs::core::otfs_waveform waveform{
        config,
        mode};

    otfs::core::aligned_buffer<
        otfs::core::sample_t> time_domain{
            waveform.time_domain_samples()};

    otfs::core::aligned_buffer<
        otfs::core::sample_t> restored{
            config.dd_cells()};

    if (!waveform.modulate(
            input,
            time_domain.span())) {
        return false;
    }

    if (!waveform.demodulate(
            time_domain.span(),
            restored.span())) {
        return false;
    }

    const double error =
        relative_error(
            restored.span(),
            input);

    return std::isfinite(error) &&
           error < 3e-6;
}

} // namespace

int main()
{
    constexpr otfs::core::waveform_config config{};

    otfs::core::aligned_buffer<
        otfs::core::sample_t> delay_doppler{
            config.dd_cells()};

    for (std::size_t doppler = 0;
         doppler < config.doppler_bins;
         ++doppler) {
        for (std::size_t delay = 0;
             delay < config.delay_bins;
             ++delay) {
            const double phase =
                2.0 * std::numbers::pi *
                static_cast<double>(
                    3U * delay + 5U * doppler) /
                static_cast<double>(
                    config.dd_cells());

            delay_doppler[
                otfs::core::column_major_index(
                    delay,
                    doppler,
                    config.delay_bins)] = {
                        static_cast<float>(
                            std::cos(phase)),
                        static_cast<float>(
                            std::sin(phase))
                    };
        }
    }

    constexpr std::array modes{
        otfs::core::guard_interval::cyclic_prefix,
        otfs::core::guard_interval::reduced_cyclic_prefix,
        otfs::core::guard_interval::zero_padding,
        otfs::core::guard_interval::reduced_zero_padding,
        otfs::core::guard_interval::none
    };

    for (const auto mode : modes) {
        if (!run_round_trip(
                mode,
                config,
                delay_doppler.span())) {
            return EXIT_FAILURE;
        }
    }

    otfs::core::otfs_waveform cp{
        config,
        otfs::core::guard_interval::cyclic_prefix};

    otfs::core::otfs_waveform rcp{
        config,
        otfs::core::guard_interval::reduced_cyclic_prefix};

    otfs::core::otfs_waveform zp{
        config,
        otfs::core::guard_interval::zero_padding};

    otfs::core::otfs_waveform rzp{
        config,
        otfs::core::guard_interval::reduced_zero_padding};

    otfs::core::otfs_waveform none{
        config,
        otfs::core::guard_interval::none};

    if (cp.time_domain_samples() !=
            (config.delay_bins +
             config.zero_padding_samples) *
            config.doppler_bins ||
        zp.time_domain_samples() !=
            cp.time_domain_samples() ||
        rcp.time_domain_samples() !=
            config.dd_cells() +
            config.zero_padding_samples ||
        rzp.time_domain_samples() !=
            rcp.time_domain_samples() ||
        none.time_domain_samples() !=
            config.dd_cells()) {
        return EXIT_FAILURE;
    }

    if (cp.demodulation_offset() !=
            config.zero_padding_samples ||
        rcp.demodulation_offset() !=
            config.zero_padding_samples ||
        zp.demodulation_offset() != 0U ||
        rzp.demodulation_offset() != 0U ||
        none.demodulation_offset() != 0U) {
        return EXIT_FAILURE;
    }

    otfs::core::aligned_buffer<
        otfs::core::sample_t> zp_time{
            zp.time_domain_samples()};

    if (!zp.modulate(
            delay_doppler.span(),
            zp_time.span())) {
        return EXIT_FAILURE;
    }

    for (std::size_t slot = 0;
         slot < config.doppler_bins;
         ++slot) {
        const std::size_t slot_offset =
            slot *
            (config.delay_bins +
             config.zero_padding_samples);

        for (std::size_t pad = 0;
             pad < config.zero_padding_samples;
             ++pad) {
            if (zp_time[
                    slot_offset +
                    config.delay_bins +
                    pad] !=
                otfs::core::sample_t{}) {
                return EXIT_FAILURE;
            }
        }
    }

    otfs::core::aligned_buffer<
        otfs::core::sample_t> rzp_time{
            rzp.time_domain_samples()};

    if (!rzp.modulate(
            delay_doppler.span(),
            rzp_time.span())) {
        return EXIT_FAILURE;
    }

    for (std::size_t index = config.dd_cells();
         index < rzp_time.size();
         ++index) {
        if (rzp_time[index] !=
            otfs::core::sample_t{}) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
