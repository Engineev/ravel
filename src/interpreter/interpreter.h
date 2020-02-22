#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <unordered_set>

#include "linker/interpretable.h"

namespace ravel {

struct InstWeight {
  InstWeight() = default;
  InstWeight(std::size_t simple, std::size_t mul, std::size_t br,
             std::size_t div, std::size_t mem)
      : simple(simple), mul(mul), br(br), div(div), mem(mem) {}

  std::size_t simple = 1;
  std::size_t mul = 4;
  std::size_t br = 8;
  std::size_t div = 8;
  std::size_t mem = 64;
  // TODO: cache
  std::size_t libcIO = 64;
  std::size_t libcMem = 128;
};

class Interpreter {
public:
  Interpreter(const Interpretable &interpretable, FILE *in, FILE *out,
              InstWeight instWeight)
      : interpretable(interpretable), in(in), out(out), instWeight(instWeight) {
  }

  void interpret();

  std::uint32_t getReturnCode() const;

  bool hasMemoryLeak() const { return !malloced.empty(); }

  std::size_t getTimeConsumed() const {
    return instCnt.simple * instWeight.simple + instCnt.mul * instWeight.mul +
           instCnt.br * instWeight.br + instCnt.div * instWeight.div +
           instCnt.mem * instWeight.mem + instCnt.libcIO * instWeight.libcIO +
           instCnt.libcMem * instWeight.libcMem;
  }

private:
  void load();

  void simulate(const std::shared_ptr<inst::Instruction> &inst);

  void simulateLibCFunc(libc::Func funcN);

private:
  const Interpretable &interpretable;

  std::array<std::int32_t, 32> regs = {0};
  std::int32_t pc = 0;
  std::vector<std::byte> storage;
  std::size_t heapPtr;
  std::unordered_set<std::size_t> malloced;

  FILE *in;
  FILE *out;
  InstWeight instWeight;
  struct InstCnt {
    std::size_t simple = 1;
    std::size_t mul = 4;
    std::size_t br = 8;
    std::size_t div = 8;
    std::size_t mem = 64;
    std::size_t libcIO = 64;
    std::size_t libcMem = 128;
  } instCnt;
};

} // namespace ravel
