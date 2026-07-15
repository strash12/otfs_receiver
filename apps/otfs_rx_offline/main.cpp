#include "otfs/core/types.hpp"
#include "otfs/core/version.hpp"
#include "otfs/io/iq_format.hpp"

#include <iostream>

int main()
{
    constexpr otfs::core::dimensions d{
        .delay_bins = 64,
        .doppler_bins = 32,
        .guard_samples = 12
    };

    std::cout
        << "OTFS receiver " << otfs::core::version() << '\n'
        << "DD cells: " << d.dd_cells() << '\n'
        << "Frame samples: " << d.frame_samples() << '\n'
        << "Default IQ format: "
        << otfs::io::to_string(
               otfs::io::iq_format::complex_float32)
        << '\n';

    return 0;
}
