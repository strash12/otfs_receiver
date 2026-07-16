#include "otfs/core/channel/fid_operator.hpp"
#include "otfs/core/equalization/cg_lmmse.hpp"
#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/model/channel.hpp"
#include "otfs/core/types.hpp"

#include <cmath>
#include <cstdlib>
#include <random>

int main()
{
    constexpr std::size_t length = 512U;

    otfs::core::fid_operator op{
        {.frame_length = length,
         .sample_rate_hz = 0.96e6}
    };

    const otfs::core::channel_path channel[] = {
        {{0.90F, 0.10F}, 0.0, 0.0}
    };

    otfs::core::aligned_buffer<
        otfs::core::sample_t> transmitted{length};

    otfs::core::aligned_buffer<
        otfs::core::sample_t> received{length};

    otfs::core::aligned_buffer<
        otfs::core::sample_t> estimated{length};

    std::mt19937 generator{7U};
    std::normal_distribution<float> normal{
        0.0F,
        1.0F / std::sqrt(2.0F)
    };

    for (auto& value : transmitted) {
        value = {
            normal(generator),
            normal(generator)
        };
    }

    if (!op.apply(
            transmitted.span(),
            channel,
            received.span())) {
        return EXIT_FAILURE;
    }

    const otfs::core::cg_lmmse_config config{
        .maximum_iterations = 30U,
        .tolerance = 1e-7,
        .lambda_floor = 1e-8,
        .estimation_error_scale = 1.0
    };

    otfs::core::cg_lmmse_equalizer equalizer{
        length,
        config};

    const auto result = equalizer.equalize(
        op,
        channel,
        received.span(),
        estimated.span(),
        1e-8,
        0.0);

    if (!result.valid ||
        !result.converged ||
        result.breakdown ||
        result.relative_residual > 1e-6) {
        return EXIT_FAILURE;
    }

    double error = 0.0;
    double energy = 0.0;

    for (std::size_t index = 0;
         index < length;
         ++index) {
        error += static_cast<double>(
            std::norm(
                estimated[index] -
                transmitted[index]));

        energy += static_cast<double>(
            std::norm(transmitted[index]));
    }

    const double relative_error =
        std::sqrt(error / energy);

    if (relative_error > 1e-4) {
        return EXIT_FAILURE;
    }

    const double lambda =
        otfs::core::cg_lmmse_equalizer::
            adaptive_lambda(
                1e-3,
                4e-3,
                config);

    if (std::abs(lambda - 4e-3) > 1e-15) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
