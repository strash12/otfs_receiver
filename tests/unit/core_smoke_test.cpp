#include "otfs/core/types.hpp"

#include <cstdlib>

int main()
{
    constexpr otfs::core::dimensions d{
        .delay_bins = 64,
        .doppler_bins = 32,
        .guard_samples = 12
    };

    if (d.dd_cells() != 2048) {
        return EXIT_FAILURE;
    }

    if (d.frame_samples() != 2432) {
        return EXIT_FAILURE;
    }

    if (otfs::core::column_major_index(3, 2, 64) != 131) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
