#include "otfs/core/fft/fft_plan.hpp"
#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/types.hpp"

#include <cmath>
#include <cstdlib>
#include <numbers>
#include <type_traits>
#include <utility>

namespace {

[[nodiscard]] double relative_error(
    std::span<const otfs::core::sample_t> lhs,
    std::span<const otfs::core::sample_t> rhs)
{
    double numerator = 0.0;
    double denominator = 0.0;

    for (std::size_t index = 0;
         index < lhs.size();
         ++index) {
        const auto difference =
            lhs[index] - rhs[index];

        numerator += static_cast<double>(
            std::norm(difference));

        denominator += static_cast<double>(
            std::norm(rhs[index]));
    }

    return std::sqrt(
        numerator /
        std::max(denominator, 1e-30));
}

} // namespace

int main()
{
    using otfs::core::aligned_buffer;
    using otfs::core::fft_direction;
    using otfs::core::fft_plan;
    using otfs::core::sample_t;

    static_assert(
        !std::is_copy_constructible_v<fft_plan>);
    static_assert(
        !std::is_copy_assignable_v<fft_plan>);
    static_assert(
        std::is_nothrow_move_constructible_v<
            fft_plan>);
    static_assert(
        std::is_nothrow_move_assignable_v<
            fft_plan>);

    constexpr std::size_t length = 1024U;

    aligned_buffer<sample_t> input{length};
    aligned_buffer<sample_t> spectrum{length};
    aligned_buffer<sample_t> restored{length};

    for (std::size_t index = 0;
         index < length;
         ++index) {
        const double x =
            static_cast<double>(index);

        const double phase1 =
            2.0 * std::numbers::pi *
            37.0 * x /
            static_cast<double>(length);

        const double phase2 =
            2.0 * std::numbers::pi *
            113.0 * x /
            static_cast<double>(length);

        input[index] = sample_t{
            static_cast<float>(
                std::cos(phase1) +
                0.25 * std::cos(phase2)),
            static_cast<float>(
                std::sin(phase1) -
                0.10 * std::sin(phase2))
        };
    }

    fft_plan forward{
        length,
        fft_direction::forward};

    fft_plan inverse{
        length,
        fft_direction::inverse};

    forward.execute(
        input.span(),
        spectrum.span());

    inverse.execute(
        spectrum.span(),
        restored.span());

    const double error =
        relative_error(
            restored.span(),
            input.span());

    if (!std::isfinite(error) ||
        error >= 2e-6) {
        return EXIT_FAILURE;
    }

    fft_plan moved{std::move(forward)};

    if (moved.length() != length ||
        moved.direction() !=
            fft_direction::forward ||
        forward.length() != 0U) {
        return EXIT_FAILURE;
    }

    aligned_buffer<sample_t> spectrum2{length};

    moved.execute(
        input.span(),
        spectrum2.span());

    const double spectrum_error =
        relative_error(
            spectrum2.span(),
            spectrum.span());

    if (!std::isfinite(spectrum_error) ||
        spectrum_error >= 1e-7) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
