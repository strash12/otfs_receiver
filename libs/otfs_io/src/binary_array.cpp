#include "otfs/io/binary_array.hpp"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <vector>

namespace otfs::io {

bool read_float64_le(
    const std::filesystem::path& path,
    std::vector<double>& output)
{
    std::ifstream stream{
        path,
        std::ios::binary | std::ios::ate};

    if (!stream) {
        return false;
    }

    const auto end = stream.tellg();

    if (end < 0) {
        return false;
    }

    const auto bytes =
        static_cast<std::size_t>(end);

    if (bytes % sizeof(double) != 0U) {
        return false;
    }

    stream.seekg(0, std::ios::beg);

    std::vector<unsigned char> raw(bytes);

    if (bytes > 0U &&
        !stream.read(
            reinterpret_cast<char*>(raw.data()),
            static_cast<std::streamsize>(bytes))) {
        return false;
    }

    output.resize(bytes / sizeof(double));

    for (std::size_t index = 0;
         index < output.size();
         ++index) {
        const auto offset =
            index * sizeof(double);

        std::uint64_t word{};

        for (std::size_t byte = 0;
             byte < sizeof(double);
             ++byte) {
            word |=
                static_cast<std::uint64_t>(
                    raw[offset + byte]) <<
                (8U * byte);
        }

        output[index] =
            std::bit_cast<double>(word);
    }

    return true;
}

} // namespace otfs::io
