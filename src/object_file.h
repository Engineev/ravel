#pragma once

#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "instructions.h"

namespace ravel {

class ObjectFile {
public:
private:
  std::vector<std::shared_ptr<inst::Instruction>> insts;
  std::vector<std::byte> storage;
  std::unordered_set<std::string> externalLabels;
  std::unordered_map<std::string, std::size_t> globalLabel2Pos;
};

} // namespace ravel
