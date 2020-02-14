#pragma once

#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "instructions.h"

namespace ravel {

// ObjectFile contains the following fields:
// * std::vector<std::byte> storage: the content of the object file, including
//     instructions, initialized and uninitialized data. Note that we do not
//     use the RISC-V instruction encoding here. See the next section for
//     details.
// * std::vector<std::pair<std::shared_ptr<inst::Instruction>, std::size_t>>
//     instsAndPos: instructions and their corresponding position in [storage].
// * std::unordered_map<std::string, std::size_t> globalLabel2Pos: the map
//     from global labels to their position in [storage].
// * std::unordered_map<inst::Instruction::Id, std::string>
//     containsExternalLabel: In the object file, some instructions still
//     contain external labels which are going to be resolved in the linking
//     phrase. This map contains their ID and the name of label.
// * std::unordered_set<inst::Instruction::Id> containsRelocationFunc: Some
//     instructions contains relocation functions such as %hi. So the
//     immediate in the instruction should be recomputed.
//
// The instruction encoding scheme:
//     Instructions are represented by their position in [instAndPos]. Namely,
//     for a word representing a instruction in [storage], we need to cast it
//     as a 32-bit unsigned integer and interpreted as the index of the
//     instruction. Example:
//       auto pos = *(std::uint32_t *)(storage.data() + pc);
//       auto inst = instsAndPos.at(pos).first;
class ObjectFile {
public:
  ObjectFile(
      std::vector<std::byte> storage,
      std::vector<std::pair<std::shared_ptr<inst::Instruction>, std::size_t>>
          instsAndPos,
      std::unordered_map<std::string, std::size_t> globalLabel2Pos,
      std::unordered_map<inst::Instruction::Id, std::string>
          containsExternalLabel,
      std::unordered_set<inst::Instruction::Id> containsRelocationFunc)
      : storage(std::move(storage)), instsAndPos(std::move(instsAndPos)),
        globalLabel2Pos(std::move(globalLabel2Pos)),
        containsExternalLabel(std::move(containsExternalLabel)),
        containsRelocationFunc(std::move(containsRelocationFunc)) {}

  const std::vector<std::byte> &getStorage() const { return storage; }

  const std::vector<std::pair<std::shared_ptr<inst::Instruction>, std::size_t>>
      &getInstsAndPos() const {
    return instsAndPos;
  }

  const std::unordered_map<std::string, std::size_t> &
  getGlobalLabel2Pos() const {
    return globalLabel2Pos;
  }

  const std::unordered_map<inst::Instruction::Id, std::string> &
  getContainsExternalLabel() const {
    return containsExternalLabel;
  }

  const std::unordered_set<inst::Instruction::Id> &
  getContainsRelocationFunc() const {
    return containsRelocationFunc;
  }

private:
  std::vector<std::byte> storage;
  std::vector<std::pair<std::shared_ptr<inst::Instruction>, std::size_t>>
      instsAndPos;
  std::unordered_map<std::string, std::size_t> globalLabel2Pos;
  std::unordered_map<inst::Instruction::Id, std::string> containsExternalLabel;
  std::unordered_set<inst::Instruction::Id> containsRelocationFunc;
};

} // namespace ravel
