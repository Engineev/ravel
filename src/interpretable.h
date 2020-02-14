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
// The next 36 bytes are just placeholders for C library functions. When the
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
  Puts = 12,
};

inline const std::unordered_map<std::string, Func> &getName2Pos() {
  using namespace std::string_literals;
  static std::unordered_map<std::string, Func> mp = {{"puts", Func::Puts}};
  return mp;
}

} // namespace libc

} // namespace ravel
