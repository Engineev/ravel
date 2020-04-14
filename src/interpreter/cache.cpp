#include "cache.h"

#include "error.h"

namespace ravel {

std::pair<std::uint32_t /* val */, bool /* hit */>
Cache::get(std::size_t addr) {
  if (!(0 <= addr && addr + 3 < memory.size())) {
    throw InvalidAddress(addr);
  }
  if (disabled)
    return {*(std::uint32_t *)(memory.data() + addr), false};
  for (auto &line : lines) {
    if (!line.valid)
      continue;
    if ((Mask & addr) != (Mask & line.addr))
      continue;
    // hit
    line.lastUsed = cycles;
    return {*(std::uint32_t *)(memory.data() + addr), true};
  }
  // miss
  auto &line = getEmptyLine();
  line.lastUsed = cycles;
  line.valid = true;
  line.addr = addr & Mask;
  return {*(std::uint32_t *)(memory.data() + addr), false};
}

Cache::Line &Cache::getEmptyLine() {
  std::size_t resIdx = lines.size();
  for (std::size_t i = 0; i < lines.size(); ++i) {
    if (!lines[i].valid)
      return lines[i];
    if (cycles >= lines[i].lastUsed + 32)
      resIdx = i;
  }
  if (resIdx != lines.size())
    return lines[resIdx];
  auto &res = lines[0];
  victimIdx += 7;
  victimIdx %= 16;
  res.valid = false;
  return res;
}

} // namespace ravel
