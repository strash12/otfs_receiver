#include "otfs/core/synchronization/chirp_synchronizer.hpp"

#include <algorithm>
#include <cmath>
#include <complex>
#include <limits>
#include <stdexcept>

namespace otfs::core {

chirp_synchronizer::chirp_synchronizer(
    std::span<const sample_t> chirp_reference,
    const chirp_sync_config& config,
    std::size_t maximum_capture_length)
    : config_(config),
      maximum_capture_length_(maximum_capture_length),
      chirp_length_(chirp_reference.size()),
      fft_length_(
          next_power_of_two(
              maximum_capture_length +
              chirp_reference.size() - 1U)),
      forward_(fft_length_, fft_direction::forward),
      inverse_(fft_length_, fft_direction::inverse),
      chirp_reference_(chirp_reference.size()),
      kernel_spectrum_(fft_length_),
      fft_input_(fft_length_),
      capture_spectrum_(fft_length_),
      product_spectrum_(fft_length_),
      convolution_(fft_length_),
      matched_filter_magnitude_(maximum_capture_length),
      median_workspace_(maximum_capture_length)
{
    if (!config_.valid(
            chirp_length_,
            maximum_capture_length_)) {
        throw std::invalid_argument(
            "Invalid chirp synchronizer configuration");
    }

    std::copy(
        chirp_reference.begin(),
        chirp_reference.end(),
        chirp_reference_.begin());

    fft_input_.fill({});

    // MATLAB:
    // fft(conj(flipud(chirp_ref)), nfft)
    for (std::size_t index = 0;
         index < chirp_length_;
         ++index) {
        fft_input_[index] =
            std::conj(
                chirp_reference_[
                    chirp_length_ - 1U - index]);
    }

    forward_.execute(
        fft_input_.span(),
        kernel_spectrum_.span());
}

chirp_sync_result chirp_synchronizer::process(
    std::span<const sample_t> capture) noexcept
{
    chirp_sync_result result{};

    if (capture.size() < config_.payload_length ||
        capture.size() > maximum_capture_length_) {
        return result;
    }

    fft_input_.fill({});

    std::copy(
        capture.begin(),
        capture.end(),
        fft_input_.begin());

    forward_.execute(
        fft_input_.span(),
        capture_spectrum_.span());

    for (std::size_t index = 0;
         index < fft_length_;
         ++index) {
        product_spectrum_[index] =
            capture_spectrum_[index] *
            kernel_spectrum_[index];
    }

    inverse_.execute(
        product_spectrum_.span(),
        convolution_.span());

    for (std::size_t index = 0;
         index < capture.size();
         ++index) {
        matched_filter_magnitude_[index] =
            std::abs(convolution_[index]);
    }

    // MATLAB 1-based:
    // firstPeak = Lc
    // lastPeak  = N - Lpayload + Lc
    //
    // C++ 0-based:
    const std::size_t first_peak =
        chirp_length_ - 1U;

    const std::size_t last_peak =
        capture.size() -
        config_.payload_length +
        chirp_length_ - 1U;

    if (last_peak < first_peak ||
        last_peak >= capture.size()) {
        return result;
    }

    std::size_t peak_index = first_peak;
    float peak_value =
        matched_filter_magnitude_[first_peak];

    for (std::size_t index = first_peak + 1U;
         index <= last_peak;
         ++index) {
        if (matched_filter_magnitude_[index] >
            peak_value) {
            peak_value =
                matched_filter_magnitude_[index];
            peak_index = index;
        }
    }

    const std::size_t candidate_start =
        peak_index - chirp_length_ + 1U;

    double correlation_real = 0.0;
    double correlation_imag = 0.0;
    double reference_energy = 0.0;
    double candidate_energy = 0.0;

    for (std::size_t index = 0;
         index < chirp_length_;
         ++index) {
        const auto reference =
            chirp_reference_[index];

        const auto candidate =
            capture[candidate_start + index];

        const auto product =
            std::conj(reference) * candidate;

        correlation_real +=
            static_cast<double>(product.real());

        correlation_imag +=
            static_cast<double>(product.imag());

        reference_energy +=
            static_cast<double>(std::norm(reference));

        candidate_energy +=
            static_cast<double>(std::norm(candidate));
    }

    const double correlation_magnitude =
        std::hypot(
            correlation_real,
            correlation_imag);

    const double denominator =
        std::sqrt(
            reference_energy *
            candidate_energy) +
        std::numeric_limits<double>::epsilon();

    const float rho =
        static_cast<float>(
            correlation_magnitude / denominator);

    const std::size_t exclusion =
        std::max<std::size_t>(
            8U,
            static_cast<std::size_t>(
                std::lround(
                    0.05 *
                    static_cast<double>(
                        chirp_length_))));

    const float background_power =
        compute_background_median(
            capture.size(),
            peak_index,
            exclusion);

    const double peak_power =
        static_cast<double>(peak_value) *
        static_cast<double>(peak_value);

    const float peak_db =
        static_cast<float>(
            10.0 *
            std::log10(
                peak_power /
                (static_cast<double>(
                     background_power) +
                 std::numeric_limits<double>::epsilon())));

    result.chirp_start = candidate_start;
    result.matched_filter_peak = peak_index;
    result.peak_db = peak_db;
    result.rho = rho;
    result.peak_value = peak_value;

    result.found =
        std::isfinite(peak_db) &&
        peak_db >= config_.minimum_peak_db &&
        std::isfinite(rho) &&
        rho >= config_.minimum_rho;

    return result;
}

std::span<const float>
chirp_synchronizer::matched_filter_magnitude() const noexcept
{
    return matched_filter_magnitude_.span();
}

std::size_t
chirp_synchronizer::chirp_length() const noexcept
{
    return chirp_length_;
}

std::size_t
chirp_synchronizer::fft_length() const noexcept
{
    return fft_length_;
}

std::size_t
chirp_synchronizer::maximum_capture_length() const noexcept
{
    return maximum_capture_length_;
}

std::size_t chirp_synchronizer::next_power_of_two(
    std::size_t value) noexcept
{
    if (value <= 1U) {
        return 1U;
    }

    --value;

    for (std::size_t shift = 1U;
         shift < sizeof(std::size_t) * 8U;
         shift <<= 1U) {
        value |= value >> shift;
    }

    return value + 1U;
}

float chirp_synchronizer::compute_background_median(
    std::size_t capture_length,
    std::size_t peak_index,
    std::size_t exclusion) noexcept
{
    const std::size_t exclusion_begin =
        peak_index > exclusion
            ? peak_index - exclusion
            : 0U;

    const std::size_t exclusion_end =
        std::min(
            capture_length - 1U,
            peak_index + exclusion);

    std::size_t count = 0U;

    for (std::size_t index = 0;
         index < capture_length;
         ++index) {
        if (index >= exclusion_begin &&
            index <= exclusion_end) {
            continue;
        }

        const float magnitude =
            matched_filter_magnitude_[index];

        median_workspace_[count] =
            magnitude * magnitude;

        ++count;
    }

    if (count == 0U) {
        return 0.0F;
    }

    auto begin = median_workspace_.begin();
    auto middle = begin +
        static_cast<std::ptrdiff_t>(count / 2U);

    std::nth_element(
        begin,
        middle,
        begin + static_cast<std::ptrdiff_t>(count));

    if ((count % 2U) != 0U) {
        return *middle;
    }

    const float upper = *middle;

    const auto lower_it = std::max_element(
        begin,
        middle);

    const float lower =
        lower_it != middle
            ? *lower_it
            : upper;

    return 0.5F * (lower + upper);
}

} // namespace otfs::core
