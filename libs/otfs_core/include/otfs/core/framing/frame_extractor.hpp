#pragma once

#include "otfs/core/resampling/integer_decimator.hpp"
#include "otfs/core/resampling/sample_rate_config.hpp"
#include "otfs/core/types.hpp"

#include <cstddef>
#include <span>

namespace otfs::core {

struct frame_extractor_config {
    sample_rate_config sample_rates{};
    std::size_t processing_frame_samples{2432};
    std::size_t frame_offset_input_samples{};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return sample_rates.valid() &&
               processing_frame_samples > 0U;
    }

    [[nodiscard]] constexpr std::size_t
    input_frame_samples() const noexcept
    {
        return sample_rates.to_input_samples(
            processing_frame_samples);
    }
};

class frame_extractor final {
public:
    frame_extractor(
        const frame_extractor_config& frame_config,
        const integer_decimator_config& decimator_config);

    [[nodiscard]] bool process(
        std::span<const sample_t> corrected_capture,
        std::size_t chirp_start,
        std::span<sample_t> input_rate_frame,
        std::span<sample_t> processing_rate_frame) const noexcept;

    [[nodiscard]] const frame_extractor_config&
    config() const noexcept;

private:
    frame_extractor_config config_{};
    integer_decimator decimator_;
};

} // namespace otfs::core
