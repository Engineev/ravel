#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include "interpretable.h"

namespace ravel {

class Interpreter {
public:
  explicit Interpreter(const Interpretable &interpretable)
      : interpretable(interpretable) {}

  void interpret();

  std::uint32_t getReturnCode() const;

private:
  void load();

  void simulate(const std::shared_ptr<inst::Instruction> &inst);

private:
  const Interpretable &interpretable;

  std::array<std::int32_t, 32> regs = {0};
  std::int32_t pc = 0;
  std::vector<std::byte> storage;
};

} // namespace ravel
