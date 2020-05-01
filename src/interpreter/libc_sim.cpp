#include "libc_sim.h"

#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>

#include "container_utils.h"
#include "error.h"

// IO
namespace ravel::libc {
namespace {

std::size_t countFormatSigns(const std::string &fmtStr) {
  std::size_t cnt = 0;
  for (auto iter = fmtStr.begin(); iter != fmtStr.end(); ++iter) {
    auto ch = *iter;
    if (ch != '%')
      continue;
    ++cnt;
    if (iter + 1 != fmtStr.end() && *(iter + 1) == '%')
      ++iter;
  }
  return cnt;
}

// return the formatted string and the number of characters written (if
// successful) or -1 if an error occurred
std::pair<std::string, int> sprintfImpl(const std::string &fmtStr,
                                        const std::vector<std::uint32_t> &args,
                                        const std::vector<std::byte> &storage) {
  std::string formattedStr;
  std::size_t curArgIdx = 0;
  for (std::size_t i = 0; i < fmtStr.size(); ++i) {
    auto ch = fmtStr[i];
    if (ch != '%') {
      formattedStr.push_back(ch);
      continue;
    }
    assert(i + 1 < fmtStr.size());
    ++i;
    ch = fmtStr[i];
    if (ch == '%') {
      formattedStr.push_back('%');
      continue;
    }
    if (ch == 'd') {
      formattedStr += std::to_string((std::int32_t)args.at(curArgIdx++));
      continue;
    }
    if (ch == 's') {
      formattedStr +=
          std::string((const char *)(storage.data() + args.at(curArgIdx++)));
      continue;
    }
    throw RuntimeError("Invalid format string: " + fmtStr);
  }

  return {formattedStr, (int)formattedStr.size()};
}
} // namespace

void puts(std::array<std::uint32_t, 32> &regs, std::vector<std::byte> &storage,
          FILE *fp) {
  std::size_t pos = regs[10];
  regs[10] = std::fputs((const char *)(storage.data() + pos), fp);
  std::fputc('\n', fp);
}

void scanf(std::array<std::uint32_t, 32> &regs, std::vector<std::byte> &storage,
           FILE *fp) {
  auto fmtStr = (const char *)(storage.data() + regs[10]);
  std::size_t assigned = 0;
  bool succeeded = true;
  for (auto iter = fmtStr; *iter != '\0' && succeeded; ++iter) {
    auto fmtCh = *iter;
    if (isspace(fmtCh)) {
      int ch = std::fgetc(fp);
      while (std::isspace(ch))
        ch = std::getc(fp);
      std::ungetc(ch, fp);
      continue;
    }

    if (fmtCh == '%') {
      ++iter;
      fmtCh = *iter;
      //      assert(fmtCh == 'd');
      if (fmtCh == 'd') {
        succeeded = std::fscanf(fp, "%d",
                                (int *)(storage.data() + regs[11 + assigned]));
      } else if (fmtCh == 's') {
        succeeded = std::fscanf(fp, "%s",
                                (char *)(storage.data() + regs[11 + assigned]));
      } else {
        assert(false);
      }
      assigned += succeeded;
      continue;
    }

    char ch = std::fgetc(fp);
    while (std::isspace(ch))
      ch = std::fgetc(fp);
    succeeded = ch == fmtCh;
  }
  regs[10] = assigned;
}

void sscanf(std::array<std::uint32_t, 32> &regs,
            std::vector<std::byte> &storage) {
  auto buffer = (const char *)(storage.data() + regs[10]);
  auto fmtStr = (const char *)(storage.data() + regs[11]);
  std::size_t assigned = 0;
  bool succeeded = true;
  auto bufferIter = buffer;
  for (auto iter = fmtStr; *iter != '\0' && succeeded; ++iter) {
    auto fmtCh = *iter;
    if (isspace(fmtCh)) {
      int ch = *bufferIter;
      while (std::isspace(ch)) {
        ++bufferIter;
        ch = *bufferIter;
      }
      --bufferIter;
      continue;
    }

    if (fmtCh == '%') {
      ++iter;
      fmtCh = *iter;
      assert(fmtCh == 'd');
      succeeded = std::sscanf(bufferIter, "%d",
                              (int *)(storage.data() + regs[12 + assigned]));
      assigned += succeeded;
      continue;
    }

    char ch = *bufferIter;
    while (std::isspace(ch)) {
      ++bufferIter;
      ch = *bufferIter;
    }
    succeeded = ch == fmtCh;
  }
  regs[10] = assigned;
}

void printf(std::array<std::uint32_t, 32> &regs,
            const std::vector<std::byte> &storage, FILE *fp) {
  auto fmtStr = std::string((const char *)(storage.data() + regs[10]));
  std::size_t nFormatSigns = countFormatSigns(fmtStr);
  std::vector<std::uint32_t> arguments(regs.begin() + 11,
                                       regs.begin() + 11 + nFormatSigns);
  assert(arguments.size() <= 7);
  auto [formattedStr, nSuccChars] = sprintfImpl(fmtStr, arguments, storage);
  std::fprintf(fp, "%s", formattedStr.c_str());
  regs[10] = nSuccChars;
}

void sprintf(std::array<std::uint32_t, 32> &regs,
             std::vector<std::byte> &storage) {
  std::size_t bufferVAddr = regs[10];
  auto fmtStr = std::string((const char *)(storage.data() + regs[11]));
  std::size_t nFormatSigns = countFormatSigns(fmtStr);
  std::vector<std::uint32_t> arguments(regs.begin() + 12,
                                       regs.begin() + 12 + nFormatSigns);
  assert(arguments.size() <= 6);
  auto [formattedStr, nSuccChars] = sprintfImpl(fmtStr, arguments, storage);
  std::sprintf((char *)(storage.data() + bufferVAddr), "%s",
               formattedStr.c_str());
  regs[10] = nSuccChars;
}

void putchar(std::array<std::uint32_t, 32> &regs, FILE *fp) {
  regs[10] = std::putc(regs[10], fp);
}

} // namespace ravel::libc

namespace ravel::libc {
namespace {
// When a function manipulates a large block of memory, the instCnt should be
// increased by more than once.
// Hence, we use the rule: instCnt += size / MemSizeFactor
constexpr std::size_t MemSizeFactor = 512;
} // namespace

void malloc(std::array<std::uint32_t, 32> &regs,
            const std::vector<std::byte> &storage, std::size_t &heapPtr,
            std::unordered_set<std::size_t> &malloced,
            std::unordered_set<std::size_t> &invalidAddress,
            std::size_t &instCnt) {
  auto size = (std::size_t)regs[10];
  instCnt += size / MemSizeFactor;
  regs[10] = heapPtr;
  malloced.emplace(heapPtr);
  heapPtr += size;
  invalidAddress.emplace(heapPtr++);
  if (heapPtr % 2) {
    invalidAddress.emplace(heapPtr++);
  }
  if (heapPtr >= storage.size()) {
    throw RuntimeError("Running out of memory");
  }
}

void free(const std::array<std::uint32_t, 32> &regs,
          std::unordered_set<std::size_t> &malloced) {
  std::size_t addr = regs[10];
  assert(isIn(malloced, addr));
  malloced.erase(malloced.find(addr));
}

void memcpy(std::array<std::uint32_t, 32> &regs,
            std::vector<std::byte> &storage, std::size_t &instCnt) {
  std::size_t dest = regs[10];
  std::size_t src = regs[11];
  std::size_t cnt = regs[12];
  instCnt += cnt / MemSizeFactor;
  assert(src + cnt < storage.size() && dest + cnt < storage.size());
  std::memcpy(storage.data() + dest, storage.data() + src, cnt);
}

void strlen(std::array<std::uint32_t, 32> &regs,
            const std::vector<std::byte> &storage) {
  std::size_t strPos = regs[10];
  regs[10] = std::strlen((char *)storage.data() + strPos);
}

void strcpy(std::array<std::uint32_t, 32> &regs,
            std::vector<std::byte> &storage, std::size_t &instCnt) {
  std::size_t dest = regs[10], src = regs[11];
  std::size_t size = std::strlen((char *)storage.data() + src);
  instCnt += size / MemSizeFactor;
  std::strcpy((char *)storage.data() + dest, (char *)storage.data() + src);
}

void strcat(std::array<std::uint32_t, 32> &regs,
            std::vector<std::byte> &storage, std::size_t &instCnt) {
  auto dest = (char *)(storage.data() + regs[10]);
  auto src = (const char *)(storage.data() + regs[11]);
  std::size_t size = std::strlen(src);
  instCnt += size / MemSizeFactor;
  std::strcat(dest, src);
}

void strcmp(std::array<std::uint32_t, 32> &regs,
            std::vector<std::byte> &storage) {
  auto lhs = (const char *)(storage.data() + regs[10]);
  auto rhs = (const char *)(storage.data() + regs[11]);
  regs[10] = std::strcmp(lhs, rhs);
}

void memset(std::array<std::uint32_t, 32> &regs,
            std::vector<std::byte> &storage, std::size_t &instCnt) {
  std::size_t dest = regs[10];
  int ch = regs[11];
  std::size_t cnt = regs[12];
  instCnt += cnt / MemSizeFactor;
  std::memset(storage.data() + dest, ch, cnt);
}

} // namespace ravel::libc