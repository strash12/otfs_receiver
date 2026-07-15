#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/model/config.hpp"
#include "otfs/core/pilot/pilot_masks.hpp"
#include "otfs/core/types.hpp"
#include "otfs/core/waveform/guard_interval.hpp"
#include "otfs/core/waveform/otfs_waveform.hpp"
#include "otfs/io/reference_data.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

namespace {

using otfs::core::sample_t;

struct mode_case {
    otfs::core::guard_interval mode;
    std::string_view file_suffix;
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

[[nodiscard]] bool equal_mask(
    std::span<const std::uint8_t> lhs,
    std::span<const std::uint8_t> rhs)
{
    return lhs.size() == rhs.size() &&
           std::equal(
               lhs.begin(),
               lhs.end(),
               rhs.begin());
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr
            << "Usage: matlab_reference_test <reference-dir>\n";
        return 2;
    }

    const std::filesystem::path reference_dir{argv[1]};

    if (!std::filesystem::exists(
            reference_dir / "metadata.txt")) {
        std::cout
            << "SKIP: MATLAB reference data not generated.\n";
        return 77;
    }

    constexpr otfs::core::receiver_config config{};

    std::vector<sample_t> dd_input;

    if (!otfs::io::read_complex_float32_le(
            reference_dir / "dd_input.cf32",
            dd_input) ||
        dd_input.size() != config.waveform.dd_cells()) {
        std::cerr << "Failed to read dd_input.cf32\n";
        return EXIT_FAILURE;
    }

    constexpr std::array mode_cases{
        mode_case{
            otfs::core::guard_interval::cyclic_prefix,
            "cp"},
        mode_case{
            otfs::core::guard_interval::reduced_cyclic_prefix,
            "rcp"},
        mode_case{
            otfs::core::guard_interval::zero_padding,
            "zp"},
        mode_case{
            otfs::core::guard_interval::reduced_zero_padding,
            "rzp"},
        mode_case{
            otfs::core::guard_interval::none,
            "none"}
    };

    for (const auto& test_case : mode_cases) {
        otfs::core::otfs_waveform waveform{
            config.waveform,
            test_case.mode};

        std::vector<sample_t> matlab_time;
        std::vector<sample_t> matlab_restored;

        if (!otfs::io::read_complex_float32_le(
                reference_dir /
                    ("time_" +
                     std::string(test_case.file_suffix) +
                     ".cf32"),
                matlab_time) ||
            !otfs::io::read_complex_float32_le(
                reference_dir /
                    ("dd_restored_" +
                     std::string(test_case.file_suffix) +
                     ".cf32"),
                matlab_restored)) {
            std::cerr
                << "Failed to read MATLAB files for "
                << test_case.file_suffix << '\n';
            return EXIT_FAILURE;
        }

        otfs::core::aligned_buffer<sample_t> cpp_time{
            waveform.time_domain_samples()};

        otfs::core::aligned_buffer<sample_t> cpp_restored{
            config.waveform.dd_cells()};

        if (!waveform.modulate(
                dd_input,
                cpp_time.span()) ||
            !waveform.demodulate(
                cpp_time.span(),
                cpp_restored.span())) {
            std::cerr
                << "C++ OTFS processing failed for "
                << test_case.file_suffix << '\n';
            return EXIT_FAILURE;
        }

        const double time_error =
            relative_error(
                cpp_time.span(),
                matlab_time);

        const double restored_error =
            relative_error(
                cpp_restored.span(),
                matlab_restored);

        std::cout
            << test_case.file_suffix
            << ": time error=" << time_error
            << ", DD error=" << restored_error
            << '\n';

        if (!std::isfinite(time_error) ||
            !std::isfinite(restored_error) ||
            time_error >= 5e-6 ||
            restored_error >= 5e-6) {
            return EXIT_FAILURE;
        }
    }

    otfs::core::pilot_masks cpp_masks{config};

    std::vector<std::uint8_t> mask_data;
    std::vector<std::uint8_t> mask_guard;
    std::vector<std::uint8_t> mask_observation;
    std::vector<std::uint8_t> mask_search;
    std::vector<std::uint8_t> mask_noise;

    if (!otfs::io::read_uint8(
            reference_dir / "mask_data.u8",
            mask_data) ||
        !otfs::io::read_uint8(
            reference_dir / "mask_guard.u8",
            mask_guard) ||
        !otfs::io::read_uint8(
            reference_dir / "mask_observation.u8",
            mask_observation) ||
        !otfs::io::read_uint8(
            reference_dir / "mask_search.u8",
            mask_search) ||
        !otfs::io::read_uint8(
            reference_dir / "mask_noise.u8",
            mask_noise)) {
        std::cerr << "Failed to read MATLAB masks\n";
        return EXIT_FAILURE;
    }

    if (!equal_mask(cpp_masks.data(), mask_data) ||
        !equal_mask(cpp_masks.guard(), mask_guard) ||
        !equal_mask(
            cpp_masks.observation(),
            mask_observation) ||
        !equal_mask(cpp_masks.search(), mask_search) ||
        !equal_mask(cpp_masks.noise(), mask_noise)) {
        std::cerr << "MATLAB/C++ pilot mask mismatch\n";
        return EXIT_FAILURE;
    }

    std::cout
        << "MATLAB/C++ pilot masks: exact match\n";

    return EXIT_SUCCESS;
}
