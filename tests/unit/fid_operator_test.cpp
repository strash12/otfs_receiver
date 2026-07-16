#include "otfs/core/channel/fid_operator.hpp"
#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/model/channel.hpp"
#include "otfs/core/types.hpp"

#include <cmath>
#include <complex>
#include <cstdlib>
#include <numbers>
#include <random>

namespace {

[[nodiscard]] otfs::core::accumulator_t inner_product(
    std::span<const otfs::core::sample_t> lhs,
    std::span<const otfs::core::sample_t> rhs)
{
    otfs::core::accumulator_t value{};

    for (std::size_t index = 0;
         index < lhs.size();
         ++index) {
        value +=
            std::conj(
                static_cast<otfs::core::accumulator_t>(
                    lhs[index])) *
            static_cast<otfs::core::accumulator_t>(
                rhs[index]);
    }

    return value;
}

[[nodiscard]] double relative_difference(
    otfs::core::accumulator_t lhs,
    otfs::core::accumulator_t rhs)
{
    return std::abs(lhs - rhs) /
           std::max(
               {std::abs(lhs),
                std::abs(rhs),
                1e-30});
}

} // namespace

int main()
{
    constexpr std::size_t length = 2432U;

    otfs::core::fid_operator op{
        {
            .frame_length = length,
            .sample_rate_hz = 0.96e6
        }
    };

    otfs::core::aligned_buffer<
        otfs::core::sample_t> x{length};

    otfs::core::aligned_buffer<
        otfs::core::sample_t> y{length};

    otfs::core::aligned_buffer<
        otfs::core::sample_t> gx{length};

    otfs::core::aligned_buffer<
        otfs::core::sample_t> ghy{length};

    std::mt19937 generator{20260716U};
    std::normal_distribution<float> normal{
        0.0F,
        1.0F / std::sqrt(2.0F)
    };

    for (std::size_t index = 0;
         index < length;
         ++index) {
        x[index] = {
            normal(generator),
            normal(generator)
        };

        y[index] = {
            normal(generator),
            normal(generator)
        };
    }

    const otfs::core::channel_path paths[] = {
        {{0.82F, -0.15F}, 2.25, -0.40},
        {{0.35F,  0.28F}, 7.60,  1.35},
        {{0.18F, -0.09F}, 11.10, -1.70}
    };

    if (!op.apply(
            x.span(),
            paths,
            gx.span()) ||
        !op.apply_adjoint(
            y.span(),
            paths,
            ghy.span())) {
        return EXIT_FAILURE;
    }

    const auto left =
        inner_product(gx.span(), y.span());

    const auto right =
        inner_product(x.span(), ghy.span());

    const double adjoint_error =
        relative_difference(left, right);

    if (!std::isfinite(adjoint_error) ||
        adjoint_error > 2e-5) {
        return EXIT_FAILURE;
    }

    const otfs::core::channel_path identity[] = {
        {{1.0F, 0.0F}, 0.0, 0.0}
    };

    if (!op.apply(
            x.span(),
            identity,
            gx.span())) {
        return EXIT_FAILURE;
    }

    double identity_error = 0.0;
    double input_energy = 0.0;

    for (std::size_t index = 0;
         index < length;
         ++index) {
        identity_error +=
            static_cast<double>(
                std::norm(gx[index] - x[index]));

        input_energy +=
            static_cast<double>(
                std::norm(x[index]));
    }

    identity_error =
        std::sqrt(
            identity_error /
            std::max(input_energy, 1e-30));

    if (identity_error > 3e-6) {
        return EXIT_FAILURE;
    }

    otfs::core::fid_spectral_norm_estimator
        norm_estimator{length};

    const auto identity_norm =
        norm_estimator.estimate(
            op,
            identity,
            12U,
            1e-6);

    if (!identity_norm.valid ||
        std::abs(identity_norm.norm - 1.0) > 2e-5) {
        return EXIT_FAILURE;
    }

    const otfs::core::channel_path scaled_identity[] = {
        {{2.5F, 0.0F}, 0.0, 0.0}
    };

    const auto scaled_norm =
        norm_estimator.estimate(
            op,
            scaled_identity,
            12U,
            1e-6);

    if (!scaled_norm.valid ||
        std::abs(scaled_norm.norm - 2.5) > 5e-5) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
