#pragma once

#include "otfs/core/frontend/dc_remover.hpp"
#include "otfs/core/frontend/frontend_config.hpp"
#include "otfs/core/frontend/frontend_metrics.hpp"
#include "otfs/core/frontend/power_meter.hpp"
#include "otfs/core/frontend/rms_agc.hpp"
#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/types.hpp"

#include <cstddef>
#include <span>

namespace otfs::core {

class radio_frontend final {
public:
    radio_frontend(
        const radio_frontend_config& config,
        std::size_t maximum_block_size);

    radio_frontend(const radio_frontend&) = delete;
    radio_frontend& operator=(const radio_frontend&) = delete;
    radio_frontend(radio_frontend&&) noexcept = default;
    radio_frontend& operator=(radio_frontend&&) noexcept = default;

    void reset() noexcept;

    [[nodiscard]] bool process(
        std::span<const sample_t> input,
        std::span<sample_t> output,
        frontend_metrics& metrics) noexcept;

    [[nodiscard]] std::size_t maximum_block_size() const noexcept;

private:
    radio_frontend_config config_{};
    std::size_t maximum_block_size_{};

    dc_remover dc_remover_;
    power_meter power_meter_;
    rms_agc agc_;

    aligned_buffer<sample_t> dc_removed_;
};

} // namespace otfs::core
