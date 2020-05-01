#pragma once

#include <array>
#include <cstddef>
#include <cstdio>
#include <unordered_set>
#include <vector>

// IO
namespace ravel::libc {

void puts(std::array<std::uint32_t, 32> &regs, std::vector<std::byte> &storage,
          FILE *fp);

void scanf(std::array<std::uint32_t, 32> &regs, std::vector<std::byte> &storage,
           FILE *fp);

void sscanf(std::array<std::uint32_t, 32> &regs,
            std::vector<std::byte> &storage);

void sprintf(std::array<std::uint32_t, 32> &regs,
             std::vector<std::byte> &storage);

void printf(std::array<std::uint32_t, 32> &regs,
            const std::vector<std::byte> &storage, FILE *fp);

void putchar(std::array<std::uint32_t, 32> &regs, FILE *fp);

} // namespace ravel::libc

namespace ravel::libc {

void malloc(std::array<std::uint32_t, 32> &regs,
            const std::vector<std::byte> &storage, std::size_t &heapPtr,
            std::unordered_set<std::size_t> &malloced,
            std::unordered_set<std::size_t> &invalidAddress,
            std::size_t &instCnt);

void free(const std::array<std::uint32_t, 32> &regs,
          std::unordered_set<std::size_t> &malloced);

void memcpy(std::array<std::uint32_t, 32> &regs,
            std::vector<std::byte> &storage, std::size_t &instCnt);

void strlen(std::array<std::uint32_t, 32> &regs,
            const std::vector<std::byte> &storage);

void strcpy(std::array<std::uint32_t, 32> &regs,
            std::vector<std::byte> &storage, std::size_t &instCnt);

void strcat(std::array<std::uint32_t, 32> &regs,
            std::vector<std::byte> &storage, std::size_t &instCnt);

void strcmp(std::array<std::uint32_t, 32> &regs,
            std::vector<std::byte> &storage);

void memset(std::array<std::uint32_t, 32> &regs,
            std::vector<std::byte> &storage, std::size_t &instCnt);

} // namespace ravel::libc