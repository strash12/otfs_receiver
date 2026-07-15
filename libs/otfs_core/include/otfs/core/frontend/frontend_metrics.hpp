#pragma once

#include "otfs/core/types.hpp"

namespace otfs::core {

struct frontend_metrics {
    sample_t estimated_dc{};
    double input_rms{};
    double output_rms{};
    double input_power_dbfs{};
    double output_power_dbfs{};
    float applied_gain{1.0F};
    bool signal_present{};
};

} // namespace otfs::core
