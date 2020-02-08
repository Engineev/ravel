#pragma once

#include <cstddef>
#include <vector>

#include "instructions.h"

namespace ravel {

class Interpretable {
public:
  Interpretable(std::vector<std::byte> storage,
                std::vector<std::shared_ptr<inst::Instruction>> insts,
                std::size_t start, std::size_t end)
      : storage(std::move(storage)), insts(std::move(insts)), start(start),
        end(end) {}

  const std::vector<std::byte> &getStorage() const { return storage; }
  const std::vector<std::shared_ptr<inst::Instruction>> &getInsts() const {
    return insts;
  }
  std::size_t getStart() const { return start; }
  std::size_t getEnd() const { return end; }

private:
  std::vector<std::byte> storage;
  std::vector<std::shared_ptr<inst::Instruction>> insts;
  std::size_t start;
  std::size_t end;
};

} // namespace ravel
