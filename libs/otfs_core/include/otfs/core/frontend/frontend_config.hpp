#pragma once

#include <cstddef>

namespace otfs::core {

struct dc_removal_config {
    bool enabled{true};
    float forgetting_factor{0.995F};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return forgetting_factor >= 0.0F &&
               forgetting_factor < 1.0F;
    }
};

struct power_meter_config {
    std::size_t averaging_window{256};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return averaging_window > 0U;
    }
};

struct agc_config {
    bool enabled{false};
    float target_rms{0.25F};
    float minimum_gain{0.05F};
    float maximum_gain{20.0F};
    float attack{0.25F};
    float release{0.02F};
    float epsilon{1e-8F};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return target_rms > 0.0F &&
               minimum_gain > 0.0F &&
               maximum_gain >= minimum_gain &&
               attack > 0.0F &&
               attack <= 1.0F &&
               release > 0.0F &&
               release <= 1.0F &&
               epsilon > 0.0F;
    }
};

struct radio_frontend_config {
    dc_removal_config dc_removal{};
    power_meter_config power_meter{};
    agc_config agc{};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return dc_removal.valid() &&
               power_meter.valid() &&
               agc.valid();
    }
};

} // namespace otfs::core
