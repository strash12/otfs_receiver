#include "otfs/core/fft/fft_plan.hpp"

#include <algorithm>
#include <fftw3.h>
#include <stdexcept>
#include <utility>

namespace otfs::core {

namespace {

[[nodiscard]] int fftw_sign(fft_direction direction) noexcept
{
    return direction == fft_direction::forward
        ? FFTW_FORWARD
        : FFTW_BACKWARD;
}

} // namespace

fft_plan::fft_plan(
    std::size_t length,
    fft_direction direction)
    : length_(length),
      direction_(direction),
      input_buffer_(length),
      output_buffer_(length)
{
    if (length_ == 0U) {
        throw std::invalid_argument(
            "FFT length must be greater than zero");
    }

    plan_ = fftwf_plan_dft_1d(
        static_cast<int>(length_),
        reinterpret_cast<fftwf_complex*>(
            input_buffer_.data()),
        reinterpret_cast<fftwf_complex*>(
            output_buffer_.data()),
        fftw_sign(direction_),
        FFTW_MEASURE);

    if (plan_ == nullptr) {
        throw std::runtime_error(
            "FFTW plan creation failed");
    }
}

fft_plan::~fft_plan() noexcept
{
    release();
}

fft_plan::fft_plan(fft_plan&& other) noexcept
    : length_(std::exchange(other.length_, 0U)),
      direction_(other.direction_),
      input_buffer_(std::move(other.input_buffer_)),
      output_buffer_(std::move(other.output_buffer_)),
      plan_(std::exchange(other.plan_, nullptr))
{
}

fft_plan& fft_plan::operator=(fft_plan&& other) noexcept
{
    if (this != &other) {
        release();

        length_ = std::exchange(other.length_, 0U);
        direction_ = other.direction_;
        input_buffer_ = std::move(other.input_buffer_);
        output_buffer_ = std::move(other.output_buffer_);
        plan_ = std::exchange(other.plan_, nullptr);
    }

    return *this;
}

std::size_t fft_plan::length() const noexcept
{
    return length_;
}

fft_direction fft_plan::direction() const noexcept
{
    return direction_;
}

void fft_plan::execute(
    std::span<const sample_t> input,
    std::span<sample_t> output) noexcept
{
    if (plan_ == nullptr ||
        input.size() != length_ ||
        output.size() != length_) {
        return;
    }

    std::copy(
        input.begin(),
        input.end(),
        input_buffer_.begin());

    fftwf_execute(
        reinterpret_cast<fftwf_plan>(plan_));

    if (direction_ == fft_direction::inverse) {
        const float scale =
            1.0F / static_cast<float>(length_);

        for (std::size_t index = 0;
             index < length_;
             ++index) {
            output[index] =
                output_buffer_[index] * scale;
        }
    }
    else {
        std::copy(
            output_buffer_.begin(),
            output_buffer_.end(),
            output.begin());
    }
}

void fft_plan::release() noexcept
{
    if (plan_ != nullptr) {
        fftwf_destroy_plan(
            reinterpret_cast<fftwf_plan>(plan_));
        plan_ = nullptr;
    }

    length_ = 0U;
}

} // namespace otfs::core
