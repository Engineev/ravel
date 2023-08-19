#include "ravel/interpreter/cache.h"

#include "ravel/error.h"

namespace ravel {

std::uint32_t Cache::fetchWord(std::size_t addr) {
  std::size_t memorySize = storageEnd - storageBegin;
  if (addr + 4 > memorySize) {
    throw InvalidAddress(addr);
  }
  if (disabled) {
    miss++;
    return *(std::uint32_t *)(storageBegin + addr);
  }
  for (auto &line : lines) {
    if (!line.valid)
      continue;
    if ((Mask & addr) != (Mask & line.addr))
      continue;
    // hit
    line.lastUsed = cycles;
    hit++;
    return *(std::uint32_t *)(storageBegin + addr);
  }
  // miss
  auto &line = getEmptyLine();
  line.lastUsed = cycles;
  line.valid = true;
  line.addr = addr & Mask;
  miss++;
  return *(std::uint32_t *)(storageBegin + addr);
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
