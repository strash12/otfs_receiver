#include "otfs/core/framing/frame_extractor.hpp"

#include <algorithm>
#include <stdexcept>

namespace otfs::core {

frame_extractor::frame_extractor(
    const frame_extractor_config& frame_config,
    const integer_decimator_config& decimator_config)
    : config_(frame_config),
      decimator_(decimator_config)
{
    if (!config_.valid()) {
        throw std::invalid_argument(
            "Invalid frame extractor configuration");
    }

    if (decimator_config.factor !=
        config_.sample_rates.decimation_factor()) {
        throw std::invalid_argument(
            "Decimator factor does not match sample-rate ratio");
    }
}

bool frame_extractor::process(
    std::span<const sample_t> corrected_capture,
    std::size_t chirp_start,
    std::span<sample_t> input_rate_frame,
    std::span<sample_t> processing_rate_frame) const noexcept
{
    const std::size_t required_input =
        config_.input_frame_samples();

    if (input_rate_frame.size() != required_input ||
        processing_rate_frame.size() !=
            config_.processing_frame_samples) {
        return false;
    }

    const std::size_t frame_start =
        chirp_start +
        config_.frame_offset_input_samples;

    if (frame_start > corrected_capture.size() ||
        required_input >
            corrected_capture.size() - frame_start) {
        return false;
    }

    std::copy_n(
        corrected_capture.begin() +
            static_cast<std::ptrdiff_t>(
                frame_start),
        required_input,
        input_rate_frame.begin());

    return decimator_.process_frame(
        input_rate_frame,
        processing_rate_frame);
}

const frame_extractor_config&
frame_extractor::config() const noexcept
{
    return config_;
}

} // namespace otfs::core
