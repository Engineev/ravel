#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace ravel {

class Cache {
  struct Line;

public:
  explicit Cache(std::vector<std::byte> &memory) : memory(memory) {
    lines.resize(16);
  }

  void tick() { ++cycles; }

  std::pair<std::uint32_t /* val */, bool /* hit */> get(std::size_t addr);

  void disable() { disabled = true; }

private:
  Line &getEmptyLine();

private:
  static constexpr std::size_t SizePow = 6;
  static constexpr std::uint32_t Mask = std::uint32_t(-1) << SizePow;

  std::vector<std::byte> &memory;
  std::size_t cycles = 32;
  struct Line {
    std::size_t addr = 0;
    std::size_t lastUsed = 0;
    bool valid = false;
  };
  std::vector<Line> lines;
  bool disabled = false;
  std::size_t victimIdx = 7;
};

} // namespace ravel