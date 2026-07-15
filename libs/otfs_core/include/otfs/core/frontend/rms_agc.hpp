#pragma once

#include "otfs/core/frontend/frontend_config.hpp"
#include "otfs/core/types.hpp"

#include <span>

namespace otfs::core {

class rms_agc final {
public:
    explicit rms_agc(const agc_config& config);

    void reset() noexcept;

    void process(
        std::span<const sample_t> input,
        std::span<sample_t> output,
        double measured_rms) noexcept;

    [[nodiscard]] float gain() const noexcept;

private:
    agc_config config_{};
    float gain_{1.0F};
};

} // namespace otfs::core
