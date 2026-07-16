#include "otfs/core/channel/fid_operator.hpp"
#include "otfs/core/model/channel.hpp"
#include "otfs/core/types.hpp"
#include "otfs/io/binary_array.hpp"
#include "otfs/io/reference_data.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <vector>

namespace {

[[nodiscard]] double relative_error(
    std::span<const otfs::core::sample_t> lhs,
    std::span<const otfs::core::sample_t> rhs)
{
    double error = 0.0;
    double energy = 0.0;

    for (std::size_t index = 0;
         index < lhs.size();
         ++index) {
        error += static_cast<double>(
            std::norm(lhs[index] - rhs[index]));

        energy += static_cast<double>(
            std::norm(rhs[index]));
    }

    return std::sqrt(
        error / std::max(energy, 1e-30));
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        return 2;
    }

    const std::filesystem::path directory{argv[1]};

    if (!std::filesystem::exists(
            directory / "metadata.txt")) {
        std::cout << "SKIP: FID MATLAB references missing\n";
        return 77;
    }

    std::vector<otfs::core::sample_t> x;
    std::vector<otfs::core::sample_t> y;
    std::vector<otfs::core::sample_t> gx_ref;
    std::vector<otfs::core::sample_t> ghy_ref;
    std::vector<otfs::core::sample_t> gains;
    std::vector<double> delays;
    std::vector<double> dopplers;

    if (!otfs::io::read_complex_float32_le(
            directory / "x.cf32", x) ||
        !otfs::io::read_complex_float32_le(
            directory / "y.cf32", y) ||
        !otfs::io::read_complex_float32_le(
            directory / "gx.cf32", gx_ref) ||
        !otfs::io::read_complex_float32_le(
            directory / "ghy.cf32", ghy_ref) ||
        !otfs::io::read_complex_float32_le(
            directory / "gains.cf32", gains) ||
        !otfs::io::read_float64_le(
            directory / "delays.f64", delays) ||
        !otfs::io::read_float64_le(
            directory / "dopplers.f64", dopplers)) {
        return EXIT_FAILURE;
    }

    std::vector<otfs::core::channel_path> paths(gains.size());

    for (std::size_t index = 0;
         index < paths.size();
         ++index) {
        paths[index] = {
            gains[index],
            delays[index],
            dopplers[index]
        };
    }

    otfs::core::fid_operator op{
        {.frame_length = x.size(),
         .sample_rate_hz = 0.96e6}
    };

    std::vector<otfs::core::sample_t> gx(x.size());
    std::vector<otfs::core::sample_t> ghy(y.size());

    if (!op.apply(x, paths, gx) ||
        !op.apply_adjoint(y, paths, ghy)) {
        return EXIT_FAILURE;
    }

    const double g_error =
        relative_error(gx, gx_ref);

    const double gh_error =
        relative_error(ghy, ghy_ref);

    std::cout
        << "FID MATLAB regression:"
        << " G error=" << g_error
        << ", GH error=" << gh_error
        << '\n';

    if (g_error > 5e-6 ||
        gh_error > 5e-6) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
