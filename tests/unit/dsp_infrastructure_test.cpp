#include "otfs/core/dsp/grid_view.hpp"
#include "otfs/core/dsp/stage.hpp"
#include "otfs/core/dsp/workspace.hpp"
#include "otfs/core/fft/fft_plan_cache.hpp"
#include "otfs/core/model/config.hpp"
#include "otfs/core/pipeline/pipeline.hpp"

#include <array>
#include <cstdlib>
#include <string_view>

namespace {

class mark_stage final : public otfs::core::dsp_stage {
public:
    explicit mark_stage(
        otfs::core::processing_status target,
        bool should_fail = false) noexcept
        : target_(target),
          should_fail_(should_fail)
    {
    }

    [[nodiscard]] std::string_view name() const noexcept override {
        return "mark_stage";
    }

    otfs::core::stage_result process(
        otfs::core::frame_context& context) noexcept override
    {
        if (should_fail_) {
            return otfs::core::stage_result::failure;
        }

        context.status = target_;
        ++calls_;
        return otfs::core::stage_result::success;
    }

    [[nodiscard]] std::size_t calls() const noexcept {
        return calls_;
    }

private:
    otfs::core::processing_status target_;
    bool should_fail_{};
    std::size_t calls_{};
};

} // namespace

int main()
{
    constexpr otfs::core::receiver_config config{};

    otfs::core::otfs_workspace workspace{config};

    if (workspace.delay_doppler().size() !=
            config.waveform.dd_cells() ||
        workspace.time_frequency().size() !=
            config.waveform.dd_cells() ||
        workspace.time_domain().size() !=
            config.waveform.base_frame_samples()) {
        return EXIT_FAILURE;
    }

    workspace.delay_doppler()[0] = {1.0F, 2.0F};
    workspace.clear();

    if (workspace.delay_doppler()[0] !=
        otfs::core::sample_t{}) {
        return EXIT_FAILURE;
    }

    std::array<otfs::core::sample_t, 12> storage{};
    otfs::core::sample_grid_view grid{
        storage,
        3,
        4
    };

    grid(2, 3) = {4.0F, -1.0F};

    if (storage[11] !=
        otfs::core::sample_t{4.0F, -1.0F}) {
        return EXIT_FAILURE;
    }

    otfs::core::fft_plan_cache cache{};

    auto& forward =
        cache.get(
            64,
            otfs::core::fft_direction::forward);

    auto& forward_again =
        cache.get(
            64,
            otfs::core::fft_direction::forward);

    auto& inverse =
        cache.get(
            64,
            otfs::core::fft_direction::inverse);

    if (&forward != &forward_again ||
        &forward == &inverse ||
        cache.size() != 2U) {
        return EXIT_FAILURE;
    }

    mark_stage first{
        otfs::core::processing_status::synchronized};

    mark_stage second{
        otfs::core::processing_status::frame_extracted};

    otfs::core::pipeline<4> chain{};

    if (!chain.add(first) ||
        !chain.add(second) ||
        chain.size() != 2U) {
        return EXIT_FAILURE;
    }

    otfs::core::frame_context context{};
    const auto result = chain.process(context);

    if (result != otfs::core::stage_result::success ||
        context.status !=
            otfs::core::processing_status::frame_extracted ||
        first.calls() != 1U ||
        second.calls() != 1U) {
        return EXIT_FAILURE;
    }

    mark_stage failing{
        otfs::core::processing_status::decoded,
        true};

    otfs::core::pipeline<2> failing_chain{};
    failing_chain.add(first);
    failing_chain.add(failing);

    context.status = otfs::core::processing_status::empty;

    if (failing_chain.process(context) !=
            otfs::core::stage_result::failure ||
        context.status !=
            otfs::core::processing_status::failed) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
