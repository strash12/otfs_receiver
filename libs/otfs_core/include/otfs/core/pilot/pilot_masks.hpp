#pragma once

#include "otfs/core/memory/aligned_buffer.hpp"
#include "otfs/core/model/config.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace otfs::core {

class pilot_masks final {
public:
    explicit pilot_masks(const receiver_config& config);

    pilot_masks(const pilot_masks&) = delete;
    pilot_masks& operator=(const pilot_masks&) = delete;
    pilot_masks(pilot_masks&&) noexcept = default;
    pilot_masks& operator=(pilot_masks&&) noexcept = default;

    [[nodiscard]] std::span<const std::uint8_t> data() const noexcept;
    [[nodiscard]] std::span<const std::uint8_t> guard() const noexcept;
    [[nodiscard]] std::span<const std::uint8_t> observation() const noexcept;
    [[nodiscard]] std::span<const std::uint8_t> search() const noexcept;
    [[nodiscard]] std::span<const std::uint8_t> noise() const noexcept;

    [[nodiscard]] std::size_t data_cells() const noexcept;
    [[nodiscard]] std::size_t guard_cells() const noexcept;
    [[nodiscard]] std::size_t observation_cells() const noexcept;
    [[nodiscard]] std::size_t search_cells() const noexcept;
    [[nodiscard]] std::size_t noise_cells() const noexcept;

    [[nodiscard]] bool is_data(std::size_t index) const noexcept;
    [[nodiscard]] bool is_guard(std::size_t index) const noexcept;
    [[nodiscard]] bool is_observation(std::size_t index) const noexcept;
    [[nodiscard]] bool is_search(std::size_t index) const noexcept;
    [[nodiscard]] bool is_noise(std::size_t index) const noexcept;

private:
    static std::size_t count_nonzero(
        std::span<const std::uint8_t> mask) noexcept;

    aligned_buffer<std::uint8_t> data_;
    aligned_buffer<std::uint8_t> guard_;
    aligned_buffer<std::uint8_t> observation_;
    aligned_buffer<std::uint8_t> search_;
    aligned_buffer<std::uint8_t> noise_;

    std::size_t data_cells_{};
    std::size_t guard_cells_{};
    std::size_t observation_cells_{};
    std::size_t search_cells_{};
    std::size_t noise_cells_{};
};

} // namespace otfs::core
