#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <unordered_set>

#include "interpreter/cache.h"
#include "linker/interpretable.h"

namespace ravel {

struct InstWeight {
  InstWeight() = default;

  std::size_t simple = 1;
  std::size_t mul = 4;
  std::size_t cache = 4;
  std::size_t br = 8;
  std::size_t div = 8;
  std::size_t mem = 64;
  std::size_t libcIO = 64;
  std::size_t libcMem = 128;
};

struct InstCnt {
  std::size_t simple = 0;
  std::size_t mul = 0;
  std::size_t cache = 0;
  std::size_t br = 0;
  std::size_t div = 0;
  std::size_t mem = 0;
  std::size_t libcIO = 0;
  std::size_t libcMem = 0;
};

class Interpreter {
public:
  Interpreter(const Interpretable &interpretable, FILE *in, FILE *out,
              InstWeight instWeight)
      : interpretable(interpretable), cache(storage), in(in), out(out),
        instWeight(instWeight) {}

  void interpret();

  void disableCache() { cache.disable(); }

  void setKeepDebugInfo(bool val) { keepDebugInfo = val; }

  std::uint32_t getReturnCode() const;

  bool hasMemoryLeak() const { return !malloced.empty(); }

  std::size_t getTimeConsumed() const {
    return instCnt.simple * instWeight.simple + instCnt.mul * instWeight.mul +
           instCnt.cache * instWeight.cache + instCnt.br * instWeight.br +
           instCnt.div * instWeight.div + instCnt.mem * instWeight.mem +
           instCnt.libcIO * instWeight.libcIO +
           instCnt.libcMem * instWeight.libcMem;
  }

  const InstCnt &getInstCnt() const { return instCnt; }

  void enablePrintInstructions() { printInstructions = true; }

  void setTimeout(std::size_t newTimeout) { timeout = newTimeout; }

private:
  void load();

  void simulate(const std::shared_ptr<inst::Instruction> &inst);

  void simulateLibCFunc(libc::Func funcN);

private:
  const Interpretable &interpretable;

  std::array<std::uint32_t, 32> regs = {0};
  std::int32_t pc = 0;
  std::vector<std::byte> storage;
  Cache cache;
  std::size_t heapPtr = 0;
  std::unordered_set<std::size_t> malloced;
  std::unordered_set<std::size_t> invalidAddress;

  FILE *in;
  FILE *out;
  InstWeight instWeight;
  InstCnt instCnt;
  bool printInstructions = false;
  bool keepDebugInfo = false;
  std::size_t timeout = (std::size_t)-1;
};

} // namespace ravel
