#include "otfs/core/synchronization/synchronization_pipeline.hpp"

#include <stdexcept>

namespace otfs::core {

synchronization_pipeline::synchronization_pipeline(
    std::span<const sample_t> chirp_reference,
    const synchronization_pipeline_config& config)
    : config_(config),
      synchronizer_(
          chirp_reference,
          config.chirp,
          config.maximum_capture_length),
      coarse_estimator_(config.coarse_cfo),
      residual_estimator_(config.residual_cfo),
      coarse_corrector_(
          config.coarse_cfo.sample_rate_hz),
      residual_corrector_(
          config.residual_cfo.sample_rate_hz),
      coarse_corrected_(
          config.maximum_capture_length)
{
    if (!config_.valid(chirp_reference.size())) {
        throw std::invalid_argument(
            "Invalid synchronization pipeline configuration");
    }

    if (config_.coarse_cfo.sample_rate_hz !=
        config_.residual_cfo.sample_rate_hz) {
        throw std::invalid_argument(
            "Coarse and residual CFO sample rates must match");
    }
}

synchronization_pipeline_result
synchronization_pipeline::process(
    std::span<const sample_t> capture,
    std::span<sample_t> corrected_capture) noexcept
{
    synchronization_pipeline_result result{};

    if (capture.empty() ||
        capture.size() > config_.maximum_capture_length ||
        corrected_capture.size() != capture.size()) {
        return result;
    }

    result.coarse_sync =
        synchronizer_.process(capture);

    if (!result.coarse_sync.found) {
        return result;
    }

    const auto coarse_window =
        cfo_window(
            capture,
            result.coarse_sync.chirp_start,
            config_.coarse_cfo);

    if (coarse_window.empty()) {
        return result;
    }

    result.coarse_cfo =
        coarse_estimator_.estimate(coarse_window);

    if (!result.coarse_cfo.valid) {
        return result;
    }

    coarse_corrector_.reset();

    auto coarse_output =
        coarse_corrected_.span().first(
            capture.size());

    if (!coarse_corrector_.process(
            capture,
            coarse_output,
            result.coarse_cfo.frequency_hz)) {
        return result;
    }

    result.refined_sync =
        synchronizer_.process(coarse_output);

    if (!result.refined_sync.found) {
        return result;
    }

    const auto residual_window =
        cfo_window(
            coarse_output,
            result.refined_sync.chirp_start,
            config_.residual_cfo);

    if (residual_window.empty()) {
        return result;
    }

    result.residual_cfo =
        residual_estimator_.estimate(
            residual_window);

    if (!result.residual_cfo.valid) {
        return result;
    }

    residual_corrector_.reset();

    if (!residual_corrector_.process(
            coarse_output,
            corrected_capture,
            result.residual_cfo.frequency_hz)) {
        return result;
    }

    result.total_cfo_hz =
        result.coarse_cfo.frequency_hz +
        result.residual_cfo.frequency_hz;

    result.valid = true;
    return result;
}

std::span<const sample_t>
synchronization_pipeline::coarse_corrected_capture() const noexcept
{
    return coarse_corrected_.span();
}

std::span<const sample_t>
synchronization_pipeline::cfo_window(
    std::span<const sample_t> samples,
    std::size_t chirp_start,
    const repeated_block_cfo_config& config) const noexcept
{
    const std::size_t required =
        config.block_length *
        config.repetitions;

    const std::size_t start =
        chirp_start +
        config_.cfo_block_offset;

    if (start > samples.size() ||
        required > samples.size() - start) {
        return {};
    }

    return samples.subspan(
        start,
        required);
}

} // namespace otfs::core
