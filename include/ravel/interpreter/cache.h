#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace ravel {

class Cache {
  struct Line;

public:
  Cache(std::byte *storageBegin, std::byte *storageEnd)
      : storageBegin(storageBegin), storageEnd(storageEnd) {
    assert(storageEnd >= storageBegin);
    lines.resize(16);
  }

  void tick() { ++cycles; }

  std::uint32_t fetchWord(std::size_t addr);

  void disable() { disabled = true; }

  std::pair<std::byte *, std::byte *> getMemory() {
    return {storageBegin, storageEnd};
  }

  std::pair<std::size_t, std::size_t> getHitMiss() const { return {hit, miss}; }

  std::byte &operator[](std::size_t addr) {
    fetchWord(addr);
    assert(addr < std::size_t(storageEnd - storageBegin));
    return storageBegin[addr];
  }

  std::size_t storageSize() const { return storageEnd - storageBegin; }

private:
  Line &getEmptyLine();

private:
  static constexpr std::size_t SizePow = 6;
  static constexpr std::uint32_t Mask = std::uint32_t(-1) << SizePow;

  std::byte *const storageBegin;
  std::byte *const storageEnd;

  std::size_t cycles = 32;
  struct Line {
    std::size_t addr = 0;
    std::size_t lastUsed = 0;
    bool valid = false;
  };
  std::vector<Line> lines;
  bool disabled = false;
  std::size_t victimIdx = 7;

  std::size_t hit = 0;
  std::size_t miss = 0;
};

} // namespace ravel