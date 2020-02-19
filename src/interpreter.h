#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <unordered_set>

#include "interpretable.h"

namespace ravel {

class Interpreter {
public:
  explicit Interpreter(const Interpretable &interpretable)
      : interpretable(interpretable) {}

  void interpret();

  std::uint32_t getReturnCode() const;

  bool hasMemoryLeak() const { return !malloced.empty(); }

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
};

} // namespace ravel
