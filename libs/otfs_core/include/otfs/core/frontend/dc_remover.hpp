#pragma once

#include "otfs/core/frontend/frontend_config.hpp"
#include "otfs/core/types.hpp"

#include <span>

namespace otfs::core {

class dc_remover final {
public:
    explicit dc_remover(
        const dc_removal_config& config);

    void reset() noexcept;

    void process(
        std::span<const sample_t> input,
        std::span<sample_t> output) noexcept;

    [[nodiscard]] sample_t estimate() const noexcept;

private:
    dc_removal_config config_{};
    sample_t estimate_{};
};

} // namespace otfs::core
