#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include "instructions.h"

namespace ravel {

// The format of an interpretable:
// An interpretable consists of two parts: a header and the content.
//
// Header: (12 + 36 = 48 bytes)
// The first 12 bytes of the header are the assembly code:
//            .text
//   _start:
//            call main  ; this  will be decomposed into 2 instructions
//            nop        ; stands for the end of interpretation
// The next 36 bytes are just placeholders for 18 C library functions. When the
// program jumps to an address between 12 and 48, it will be viewed as calling
// a corresponding C library function.
class Interpretable {
public:
  static constexpr std::size_t Start = 0;
  static constexpr std::size_t End = 8;
  static constexpr std::size_t LibcFuncStart = 12;
  static constexpr std::size_t LibcFuncEnd = 48;

  Interpretable(std::vector<std::byte> storage,
                std::vector<std::shared_ptr<inst::Instruction>> insts)
      : storage(std::move(storage)), insts(std::move(insts)) {}

  const std::vector<std::byte> &getStorage() const { return storage; }
  const std::vector<std::shared_ptr<inst::Instruction>> &getInsts() const {
    return insts;
  }

private:
  std::vector<std::byte> storage;
  std::vector<std::shared_ptr<inst::Instruction>> insts;
};

namespace libc {
enum Func {
  PUTS = 12,
  SCANF = 14,
  SSCANF = 16,
  PRINTF = 18,
  SPRINTF = 20,
  PUTCHAR = 22,

  MALLOC = 24,
  FREE = 26,
  MEMCPY = 28,
  STRLEN = 30,
  STRCPY = 32,
  STRCAT = 34,
  STRCMP = 36,
  MEMSET = 38,
};

inline const std::unordered_map<std::string, Func> &getName2Pos() {
  using namespace std::string_literals;
  // clang-format off
  static std::unordered_map<std::string, Func> mp = {
      // IO
      {"puts", PUTS},
      {"scanf", SCANF},
      {"__isoc99_scanf", SCANF},
      {"sscanf", SSCANF},
      {"__isoc99_sscanf", SSCANF},
      {"printf", PRINTF},
      {"sprintf", SPRINTF},
      {"putchar", PUTCHAR},

      // mem
      {"malloc", MALLOC},
      {"free", FREE},
      {"memcpy", MEMCPY},
      {"strlen", STRLEN},
      {"strcpy", STRCPY},
      {"strcat", STRCAT},
      {"strcmp", STRCMP},
      {"memset", MEMSET},
  };
  // clang-format on
  return mp;
}

} // namespace libc

} // namespace ravel
