#pragma once

#include "otfs/core/fft/fft_direction.hpp"
#include "otfs/core/fft/fft_plan.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace otfs::core {

class fft_plan_cache final {
public:
    fft_plan_cache() = default;

    fft_plan_cache(const fft_plan_cache&) = delete;
    fft_plan_cache& operator=(const fft_plan_cache&) = delete;
    fft_plan_cache(fft_plan_cache&&) noexcept = default;
    fft_plan_cache& operator=(fft_plan_cache&&) noexcept = default;

    fft_plan& get(
        std::size_t length,
        fft_direction direction)
    {
        for (auto& entry : entries_) {
            if (entry.plan->length() == length &&
                entry.plan->direction() == direction) {
                return *entry.plan;
            }
        }

        entries_.push_back({
            .plan = std::make_unique<fft_plan>(
                length,
                direction)
        });

        return *entries_.back().plan;
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return entries_.size();
    }

private:
    struct entry {
        std::unique_ptr<fft_plan> plan;
    };

    std::vector<entry> entries_{};
};

} // namespace otfs::core
