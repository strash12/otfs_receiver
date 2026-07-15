#pragma once

#include "otfs/core/types.hpp"

#include <cassert>
#include <cstddef>
#include <span>

namespace otfs::core {

template<class T>
class grid_view final {
public:
    using value_type = T;

    constexpr grid_view() noexcept = default;

    constexpr grid_view(
        std::span<T> storage,
        std::size_t rows,
        std::size_t columns) noexcept
        : storage_(storage),
          rows_(rows),
          columns_(columns)
    {
        assert(storage_.size() == rows_ * columns_);
    }

    [[nodiscard]] constexpr std::size_t rows() const noexcept {
        return rows_;
    }

    [[nodiscard]] constexpr std::size_t columns() const noexcept {
        return columns_;
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept {
        return storage_.size();
    }

    [[nodiscard]] constexpr bool empty() const noexcept {
        return storage_.empty();
    }

    [[nodiscard]] constexpr std::span<T> storage() const noexcept {
        return storage_;
    }

    [[nodiscard]] constexpr T& operator()(
        std::size_t row,
        std::size_t column) const noexcept
    {
        assert(row < rows_);
        assert(column < columns_);
        return storage_[row + column * rows_];
    }

private:
    std::span<T> storage_{};
    std::size_t rows_{};
    std::size_t columns_{};
};

using sample_grid_view = grid_view<sample_t>;
using const_sample_grid_view = grid_view<const sample_t>;

} // namespace otfs::core
