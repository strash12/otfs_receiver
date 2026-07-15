#pragma once

#include "otfs/core/cfo/cfo_corrector.hpp"
#include "otfs/core/cfo/cfo_estimator.hpp"
#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/synchronization/chirp_synchronizer.hpp"
#include "otfs/core/types.hpp"

#include <cstddef>
#include <span>

namespace otfs::core {

struct synchronization_pipeline_config {
    chirp_sync_config chirp{};
    repeated_block_cfo_config coarse_cfo{};
    repeated_block_cfo_config residual_cfo{};
    std::size_t cfo_block_offset{};
    std::size_t maximum_capture_length{};

    [[nodiscard]] constexpr bool valid(
        std::size_t chirp_length) const noexcept
    {
        return chirp.valid(
                   chirp_length,
                   maximum_capture_length) &&
               coarse_cfo.valid() &&
               residual_cfo.valid() &&
               maximum_capture_length > 0U;
    }
};

struct synchronization_pipeline_result {
    bool valid{};
    chirp_sync_result coarse_sync{};
    cfo_estimate coarse_cfo{};
    chirp_sync_result refined_sync{};
    cfo_estimate residual_cfo{};
    double total_cfo_hz{};
};

class synchronization_pipeline final {
public:
    synchronization_pipeline(
        std::span<const sample_t> chirp_reference,
        const synchronization_pipeline_config& config);

    synchronization_pipeline(
        const synchronization_pipeline&) = delete;
    synchronization_pipeline& operator=(
        const synchronization_pipeline&) = delete;
    synchronization_pipeline(
        synchronization_pipeline&&) noexcept = default;
    synchronization_pipeline& operator=(
        synchronization_pipeline&&) noexcept = default;

    [[nodiscard]] synchronization_pipeline_result process(
        std::span<const sample_t> capture,
        std::span<sample_t> corrected_capture) noexcept;

    [[nodiscard]] std::span<const sample_t>
    coarse_corrected_capture() const noexcept;

private:
    [[nodiscard]] std::span<const sample_t> cfo_window(
        std::span<const sample_t> samples,
        std::size_t chirp_start,
        const repeated_block_cfo_config& config) const noexcept;

    synchronization_pipeline_config config_{};

    chirp_synchronizer synchronizer_;
    repeated_block_cfo_estimator coarse_estimator_;
    repeated_block_cfo_estimator residual_estimator_;
    cfo_corrector coarse_corrector_;
    cfo_corrector residual_corrector_;

    aligned_buffer<sample_t> coarse_corrected_;
};

} // namespace otfs::core
