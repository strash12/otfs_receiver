#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <span>
#include <type_traits>
#include <utility>

namespace otfs::core {

template<class T, std::size_t Alignment = 64>
class aligned_buffer final {
    static_assert(Alignment >= alignof(T));
    static_assert((Alignment & (Alignment - 1U)) == 0U);

public:
    using value_type = T;
    using size_type = std::size_t;
    using iterator = T*;
    using const_iterator = const T*;

    aligned_buffer() noexcept = default;

    explicit aligned_buffer(size_type count)
        : data_(allocate_and_construct(count)),
          size_(count)
    {
    }

    ~aligned_buffer() noexcept
    {
        destroy_and_deallocate();
    }

    aligned_buffer(const aligned_buffer&) = delete;
    aligned_buffer& operator=(const aligned_buffer&) = delete;

    aligned_buffer(aligned_buffer&& other) noexcept
        : data_(std::exchange(other.data_, nullptr)),
          size_(std::exchange(other.size_, 0U))
    {
    }

    aligned_buffer& operator=(aligned_buffer&& other) noexcept
    {
        if (this != &other) {
            destroy_and_deallocate();
            data_ = std::exchange(other.data_, nullptr);
            size_ = std::exchange(other.size_, 0U);
        }
        return *this;
    }

    [[nodiscard]] T* data() noexcept
    {
        return data_;
    }

    [[nodiscard]] const T* data() const noexcept
    {
        return data_;
    }

    [[nodiscard]] size_type size() const noexcept
    {
        return size_;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return size_ == 0U;
    }

    [[nodiscard]] static constexpr size_type alignment() noexcept
    {
        return Alignment;
    }

    [[nodiscard]] std::span<T> span() noexcept
    {
        return {data_, size_};
    }

    [[nodiscard]] std::span<const T> span() const noexcept
    {
        return {data_, size_};
    }

    [[nodiscard]] iterator begin() noexcept
    {
        return data_;
    }

    [[nodiscard]] const_iterator begin() const noexcept
    {
        return data_;
    }

    [[nodiscard]] const_iterator cbegin() const noexcept
    {
        return data_;
    }

    [[nodiscard]] iterator end() noexcept
    {
        return data_ + size_;
    }

    [[nodiscard]] const_iterator end() const noexcept
    {
        return data_ + size_;
    }

    [[nodiscard]] const_iterator cend() const noexcept
    {
        return data_ + size_;
    }

    [[nodiscard]] T& operator[](size_type index) noexcept
    {
        return data_[index];
    }

    [[nodiscard]] const T& operator[](size_type index) const noexcept
    {
        return data_[index];
    }

    void fill(const T& value)
    {
        std::fill(begin(), end(), value);
    }

    void swap(aligned_buffer& other) noexcept
    {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
    }

private:
    [[nodiscard]] static T* allocate_and_construct(size_type count)
    {
        if (count == 0U) {
            return nullptr;
        }

        const auto bytes = count * sizeof(T);
        auto* memory = static_cast<T*>(
            ::operator new(bytes, std::align_val_t{Alignment}));

        try {
            std::uninitialized_value_construct_n(memory, count);
        }
        catch (...) {
            ::operator delete(
                memory,
                std::align_val_t{Alignment});
            throw;
        }

        return memory;
    }

    void destroy_and_deallocate() noexcept
    {
        if (data_ == nullptr) {
            return;
        }

        std::destroy_n(data_, size_);
        ::operator delete(
            data_,
            std::align_val_t{Alignment});

        data_ = nullptr;
        size_ = 0U;
    }

    T* data_{};
    size_type size_{};
};

template<class T, std::size_t Alignment>
void swap(
    aligned_buffer<T, Alignment>& lhs,
    aligned_buffer<T, Alignment>& rhs) noexcept
{
    lhs.swap(rhs);
}

} // namespace otfs::core
