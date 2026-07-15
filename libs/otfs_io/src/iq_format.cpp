#include "otfs/io/iq_format.hpp"

namespace otfs::io {

std::string_view to_string(iq_format format) noexcept
{
    switch (format) {
    case iq_format::complex_float32:
        return "complex_float32";
    case iq_format::interleaved_float32:
        return "interleaved_float32";
    case iq_format::interleaved_int16:
        return "interleaved_int16";
    }
    return "unknown";
}

} // namespace otfs::io
