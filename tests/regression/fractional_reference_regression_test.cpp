#include "otfs/core/channel_estimation/fractional_reference.hpp"
#include "otfs/core/model/config.hpp"
#include "otfs/core/types.hpp"
#include "otfs/core/waveform/guard_interval.hpp"
#include "otfs/io/reference_data.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <span>
#include <vector>

namespace {

using otfs::core::sample_t;

struct reference_case {
    double delay_samples;
    double doppler_bins;
};

[[nodiscard]] double relative_error(
    std::span<const sample_t> lhs,
    std::span<const sample_t> rhs)
{
    if (lhs.size() != rhs.size()) {
        return INFINITY;
    }

    double numerator = 0.0;
    double denominator = 0.0;

    for (std::size_t index = 0;
         index < lhs.size();
         ++index) {
        numerator += static_cast<double>(
            std::norm(lhs[index] - rhs[index]));

        denominator += static_cast<double>(
            std::norm(rhs[index]));
    }

    return std::sqrt(
        numerator /
        std::max(denominator, 1e-30));
}

[[nodiscard]] double maximum_absolute_error(
    std::span<const sample_t> lhs,
    std::span<const sample_t> rhs)
{
    if (lhs.size() != rhs.size()) {
        return INFINITY;
    }

    double maximum = 0.0;

    for (std::size_t index = 0;
         index < lhs.size();
         ++index) {
        maximum = std::max(
            maximum,
            static_cast<double>(
                std::abs(lhs[index] - rhs[index])));
    }

    return maximum;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr
            << "Usage: fractional_reference_regression_test "
            << "<reference-dir>\n";
        return 2;
    }

    const std::filesystem::path reference_dir{argv[1]};

    if (!std::filesystem::exists(
            reference_dir / "metadata.txt")) {
        std::cout
            << "SKIP: fractional MATLAB references "
            << "not generated.\n";
        return 77;
    }

    constexpr otfs::core::receiver_config config{};

    std::vector<sample_t> pilot_time;

    if (!otfs::io::read_complex_float32_le(
            reference_dir / "pilot_time_zp.cf32",
            pilot_time)) {
        std::cerr
            << "Failed to read pilot_time_zp.cf32\n";
        return EXIT_FAILURE;
    }

    otfs::core::fractional_reference_generator generator{
        pilot_time,
        config,
        otfs::core::guard_interval::zero_padding};

    constexpr std::array cases{
        reference_case{0.00,  0.00},
        reference_case{1.25, -0.50},
        reference_case{5.23, -1.37},
        reference_case{9.75,  1.80},
        reference_case{13.40, -1.95}
    };

    std::vector<sample_t> cpp_reference(
        generator.observation_size());

    for (std::size_t case_index = 0;
         case_index < cases.size();
         ++case_index) {
        const auto& test_case = cases[case_index];

        std::vector<sample_t> matlab_reference;

        const auto file_name =
            "reference_" +
            (case_index < 10U ? std::string{"0"} : std::string{}) +
            std::to_string(case_index) +
            ".cf32";

        if (!otfs::io::read_complex_float32_le(
                reference_dir / file_name,
                matlab_reference)) {
            std::cerr
                << "Failed to read " << file_name << '\n';
            return EXIT_FAILURE;
        }

        if (!generator.make_observation_reference(
                test_case.delay_samples,
                test_case.doppler_bins,
                cpp_reference)) {
            std::cerr
                << "C++ reference generation failed for case "
                << case_index << '\n';
            return EXIT_FAILURE;
        }

        const double l2_error =
            relative_error(
                cpp_reference,
                matlab_reference);

        const double max_error =
            maximum_absolute_error(
                cpp_reference,
                matlab_reference);

        std::cout
            << "case " << case_index
            << ": delay=" << test_case.delay_samples
            << ", doppler=" << test_case.doppler_bins
            << ", relative_error=" << l2_error
            << ", max_error=" << max_error
            << '\n';

        if (!std::isfinite(l2_error) ||
            !std::isfinite(max_error) ||
            l2_error >= 5e-6 ||
            max_error >= 5e-5) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
