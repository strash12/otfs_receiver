#pragma once

#include "otfs/core/model/frame_context.hpp"

#include <string_view>

namespace otfs::core {

enum class stage_result {
    success,
    skipped,
    failure
};

class dsp_stage {
public:
    virtual ~dsp_stage() = default;

    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    virtual stage_result process(
        frame_context& context) noexcept = 0;
};

} // namespace otfs::core
