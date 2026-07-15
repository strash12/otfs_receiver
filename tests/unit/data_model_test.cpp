#include "otfs/core/model/config.hpp"
#include "otfs/core/model/frame_context.hpp"
#include "otfs/core/model/receiver_state.hpp"
#include "otfs/core/types.hpp"

#include <array>
#include <cstdlib>

int main()
{
    constexpr otfs::core::receiver_config config{};

    static_assert(config.waveform.dd_cells() == 2048);
    static_assert(config.waveform.base_frame_samples() == 2432);
    static_assert(config.waveform.base_sample_rate_hz() == 960'000.0);
    static_assert(config.waveform.radio_sample_rate_hz() == 1'920'000.0);
    static_assert(config.valid());

    std::array<otfs::core::sample_t, 16> capture{};
    std::array<otfs::core::sample_t, 8> frame{};
    std::array<otfs::core::sample_t, 8> dd{};

    otfs::core::frame_context context{};
    context.buffers.radio_capture = capture;
    context.buffers.baseband_frame = frame;
    context.buffers.delay_doppler_grid = dd;

    context.sequence = 7;
    context.status = otfs::core::processing_status::demodulated;
    context.header_valid = true;
    context.metrics.frame_sequence = 7;

    context.reset_metadata(8);

    if (context.sequence != 8 ||
        context.metrics.frame_sequence != 8 ||
        context.status != otfs::core::processing_status::empty ||
        context.header_valid ||
        context.buffers.radio_capture.size() != capture.size() ||
        context.buffers.baseband_frame.size() != frame.size() ||
        context.buffers.delay_doppler_grid.size() != dd.size()) {
        return EXIT_FAILURE;
    }

    constexpr std::array<otfs::core::channel_path, 2> paths{{
        {{1.0F, 0.0F}, 1.25, -0.5},
        {{0.2F, 0.1F}, 7.75, 1.125}
    }};

    context.channel = {
        .paths = paths,
        .noise_variance = 0.01,
        .quality = 0.95,
        .valid = true
    };

    if (!context.channel.valid ||
        context.channel.path_count() != 2 ||
        context.channel.paths[1].delay_samples != 7.75) {
        return EXIT_FAILURE;
    }

    otfs::core::receiver_state state{};
    state.processed_frames = 10;
    state.valid_frames = 9;
    state.requested_mode = otfs::core::receiver_mode::track;

    if (state.processed_frames != 10 ||
        state.valid_frames != 9 ||
        state.requested_mode != otfs::core::receiver_mode::track) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
