#pragma once

/* How to add support for a new libc function
 *
 *
 */

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <unordered_set>
#include <vector>

// IO
namespace ravel::libc {

void puts(std::array<std::uint32_t, 32> &regs, std::byte *storage,
          std::byte *storageEnd, FILE *fp);

void scanf(std::array<std::uint32_t, 32> &regs, std::byte *storage,
           std::byte *storageEnd, FILE *fp);

void sscanf(std::array<std::uint32_t, 32> &regs, std::byte *storage);

void sprintf(std::array<std::uint32_t, 32> &regs, std::byte *storage);

void printf(std::array<std::uint32_t, 32> &regs, const std::byte *storage,
            std::byte *storageEnd, FILE *fp);

void putchar(std::array<std::uint32_t, 32> &regs, FILE *fp);

} // namespace ravel::libc

namespace ravel::libc {

void malloc(std::array<std::uint32_t, 32> &regs, std::byte *storage,
            std::byte *storageEnd, std::size_t &heapPtr,
            std::unordered_set<std::size_t> &malloced,
            std::unordered_set<std::size_t> &invalidAddress,
            std::size_t &instCnt, bool zeroInit = false);

void free(const std::array<std::uint32_t, 32> &regs,
          std::unordered_set<std::size_t> &malloced);

void memcpy(std::array<std::uint32_t, 32> &regs, std::byte *storage,
            std::byte *storageEnd, std::size_t &instCnt);

void strlen(std::array<std::uint32_t, 32> &regs, const std::byte *storage);

void strcpy(std::array<std::uint32_t, 32> &regs, std::byte *storage,
            std::byte *storageEnd, std::size_t &instCnt);

void strcat(std::array<std::uint32_t, 32> &regs, std::byte *storage,
            std::byte *storageEnd, std::size_t &instCnt);

void strcmp(std::array<std::uint32_t, 32> &regs, std::byte *storage);

void memset(std::array<std::uint32_t, 32> &regs, std::byte *storage,
            std::byte *storageEnd, std::size_t &instCnt);

} // namespace ravel::libc
