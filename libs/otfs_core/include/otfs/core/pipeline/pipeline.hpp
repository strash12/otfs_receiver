#pragma once

#include "otfs/core/dsp/stage.hpp"

#include <array>
#include <cstddef>
#include <span>

namespace otfs::core {

template<std::size_t MaximumStages>
class pipeline final {
public:
    pipeline() noexcept = default;

    [[nodiscard]] bool add(dsp_stage& stage) noexcept {
        if (count_ >= MaximumStages) {
            return false;
        }

        stages_[count_] = &stage;
        ++count_;
        return true;
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return count_;
    }

    [[nodiscard]] bool empty() const noexcept {
        return count_ == 0U;
    }

    stage_result process(frame_context& context) noexcept {
        for (std::size_t index = 0; index < count_; ++index) {
            const auto result = stages_[index]->process(context);

            if (result == stage_result::failure) {
                context.status = processing_status::failed;
                return result;
            }
        }

        return stage_result::success;
    }

    [[nodiscard]] std::span<dsp_stage* const> stages() const noexcept {
        return {stages_.data(), count_};
    }

private:
    std::array<dsp_stage*, MaximumStages> stages_{};
    std::size_t count_{};
};

} // namespace otfs::core
