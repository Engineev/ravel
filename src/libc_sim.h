#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace ravel::libc {

void puts(std::array<std::int32_t, 32> &regs, std::vector<std::byte> &storage);

void scanf(std::array<std::int32_t, 32> &regs, std::vector<std::byte> &storage);

void printf(std::array<std::int32_t, 32> &regs,
            const std::vector<std::byte> &storage);

void putchar(std::array<std::int32_t, 32> &regs);

void malloc(std::array<std::int32_t, 32> &regs,
            const std::vector<std::byte> &storage, std::size_t &heapPtr);

} // namespace ravel::libc