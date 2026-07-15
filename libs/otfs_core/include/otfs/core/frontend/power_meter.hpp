#pragma once

#include "otfs/core/frontend/frontend_config.hpp"
#include "otfs/core/types.hpp"

#include <cstddef>
#include <span>

namespace otfs::core {

class power_meter final {
public:
    explicit power_meter(
        const power_meter_config& config);

    void reset() noexcept;

    [[nodiscard]] double rms(
        std::span<const sample_t> samples) const noexcept;

    [[nodiscard]] double power_dbfs(
        std::span<const sample_t> samples) const noexcept;

private:
    power_meter_config config_{};
};

} // namespace otfs::core
