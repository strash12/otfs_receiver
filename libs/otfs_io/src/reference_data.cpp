#include "otfs/io/reference_data.hpp"

#include <bit>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <limits>
#include <type_traits>
#include <vector>

namespace otfs::io {

namespace {

[[nodiscard]] float bytes_to_float_le(
    const unsigned char* bytes) noexcept
{
    std::uint32_t word =
        static_cast<std::uint32_t>(bytes[0]) |
        (static_cast<std::uint32_t>(bytes[1]) << 8U) |
        (static_cast<std::uint32_t>(bytes[2]) << 16U) |
        (static_cast<std::uint32_t>(bytes[3]) << 24U);

    return std::bit_cast<float>(word);
}

} // namespace

bool read_complex_float32_le(
    const std::filesystem::path& path,
    std::vector<otfs::core::sample_t>& output)
{
    std::ifstream stream{
        path,
        std::ios::binary | std::ios::ate};

    if (!stream) {
        return false;
    }

    const auto end_position = stream.tellg();

    if (end_position < 0) {
        return false;
    }

    const auto byte_count =
        static_cast<std::size_t>(end_position);

    constexpr std::size_t bytes_per_complex =
        2U * sizeof(float);

    if (byte_count % bytes_per_complex != 0U) {
        return false;
    }

    stream.seekg(0, std::ios::beg);

    std::vector<unsigned char> bytes(byte_count);

    if (!stream.read(
            reinterpret_cast<char*>(bytes.data()),
            static_cast<std::streamsize>(byte_count))) {
        return false;
    }

    output.resize(byte_count / bytes_per_complex);

    for (std::size_t index = 0;
         index < output.size();
         ++index) {
        const auto offset =
            index * bytes_per_complex;

        output[index] = {
            bytes_to_float_le(bytes.data() + offset),
            bytes_to_float_le(
                bytes.data() + offset + sizeof(float))
        };
    }

    return true;
}

bool read_uint8(
    const std::filesystem::path& path,
    std::vector<std::uint8_t>& output)
{
    std::ifstream stream{
        path,
        std::ios::binary | std::ios::ate};

    if (!stream) {
        return false;
    }

    const auto end_position = stream.tellg();

    if (end_position < 0) {
        return false;
    }

    const auto byte_count =
        static_cast<std::size_t>(end_position);

    stream.seekg(0, std::ios::beg);
    output.resize(byte_count);

    if (byte_count == 0U) {
        return true;
    }

    return static_cast<bool>(
        stream.read(
            reinterpret_cast<char*>(output.data()),
            static_cast<std::streamsize>(byte_count)));
}

} // namespace otfs::io
