#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/types.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <numeric>
#include <span>
#include <type_traits>
#include <utility>

namespace {

struct tracked_value {
    static inline int alive = 0;

    tracked_value()
    {
        ++alive;
    }

    ~tracked_value()
    {
        --alive;
    }

    tracked_value(const tracked_value&) = delete;
    tracked_value& operator=(const tracked_value&) = delete;
    tracked_value(tracked_value&&) = delete;
    tracked_value& operator=(tracked_value&&) = delete;
};

[[nodiscard]] bool is_aligned(
    const void* pointer,
    std::size_t alignment) noexcept
{
    return reinterpret_cast<std::uintptr_t>(pointer) %
               alignment ==
           0U;
}

} // namespace

int main()
{
    using otfs::core::aligned_buffer;

    static_assert(
        !std::is_copy_constructible_v<aligned_buffer<float>>);
    static_assert(
        !std::is_copy_assignable_v<aligned_buffer<float>>);
    static_assert(
        std::is_nothrow_move_constructible_v<
            aligned_buffer<float>>);
    static_assert(
        std::is_nothrow_move_assignable_v<
            aligned_buffer<float>>);

    aligned_buffer<float> empty{};
    if (!empty.empty() ||
        empty.size() != 0U ||
        empty.data() != nullptr) {
        return EXIT_FAILURE;
    }

    aligned_buffer<float> values{1024};

    if (values.size() != 1024U ||
        values.empty() ||
        !is_aligned(values.data(), 64U)) {
        return EXIT_FAILURE;
    }

    for (const auto value : values) {
        if (value != 0.0F) {
            return EXIT_FAILURE;
        }
    }

    values.fill(2.0F);

    auto view = values.span();
    static_assert(
        std::is_same_v<
            decltype(view),
            std::span<float>>);

    if (view.size() != values.size()) {
        return EXIT_FAILURE;
    }

    const float sum =
        std::accumulate(view.begin(), view.end(), 0.0F);

    if (sum != 2048.0F) {
        return EXIT_FAILURE;
    }

    float* const original_pointer = values.data();

    aligned_buffer<float> moved{std::move(values)};

    if (!values.empty() ||
        values.data() != nullptr ||
        moved.data() != original_pointer ||
        moved.size() != 1024U ||
        !is_aligned(moved.data(), 64U)) {
        return EXIT_FAILURE;
    }

    aligned_buffer<float> assigned{8};
    assigned = std::move(moved);

    if (!moved.empty() ||
        moved.data() != nullptr ||
        assigned.data() != original_pointer ||
        assigned.size() != 1024U) {
        return EXIT_FAILURE;
    }

    {
        aligned_buffer<tracked_value> tracked{17};

        if (tracked_value::alive != 17 ||
            !is_aligned(tracked.data(), 64U)) {
            return EXIT_FAILURE;
        }
    }

    if (tracked_value::alive != 0) {
        return EXIT_FAILURE;
    }

    aligned_buffer<otfs::core::sample_t, 128> iq{256};

    if (!is_aligned(iq.data(), 128U) ||
        iq.alignment() != 128U) {
        return EXIT_FAILURE;
    }

    iq[3] = {1.0F, -2.0F};

    const auto& const_iq = iq;
    const auto const_view = const_iq.span();

    static_assert(
        std::is_same_v<
            decltype(const_view),
            const std::span<const otfs::core::sample_t>>);

    if (const_view[3] != otfs::core::sample_t{1.0F, -2.0F}) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
