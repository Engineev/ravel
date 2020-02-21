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
// * std::unordered_map<std::string, std::size_t> symbolTable
// * std::unordered_set<std::string> globalSymbol
// * std::unordered_map<inst::Instruction::Id, std::string>
//     containsExternalLabel: In the object file, some instructions still
//     contain external labels which are going to be resolved in the linking
//     phrase. This map contains their ID and the name of label.
// * std::unordered_map<inst::Instruction::Id, std::string>
//     containsRelocationFunc: Some instructions contains relocation functions
//     such as %hi and this map contains them and the relocation functions.
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
      std::unordered_map<std::string, std::size_t> symbolTable,
      std::unordered_set<std::string> globalSymbol,
      std::unordered_map<inst::Instruction::Id, std::string>
          containsExternalLabel,
      std::unordered_map<inst::Instruction::Id, std::string>
          containsRelocationFunc)
      : storage(std::move(storage)), instsAndPos(std::move(instsAndPos)),
        symbolTable(std::move(symbolTable)),
        globalSymbol(std::move(globalSymbol)),
        containsExternalLabel(std::move(containsExternalLabel)),
        containsRelocationFunc(std::move(containsRelocationFunc)) {}

  const std::vector<std::byte> &getStorage() const { return storage; }
  const std::vector<std::pair<std::shared_ptr<inst::Instruction>, std::size_t>>
      &getInstsAndPos() const {
    return instsAndPos;
  }
  const std::unordered_map<std::string, std::size_t> &getSymbolTable() const {
    return symbolTable;
  }
  const std::unordered_set<std::string> &getGlobalSymbol() const {
    return globalSymbol;
  }
  const std::unordered_map<inst::Instruction::Id, std::string> &
  getContainsRelocationFunc() const {
    return containsRelocationFunc;
  }
  const std::unordered_map<inst::Instruction::Id, std::string> &
  getContainsExternalLabel() const {
    return containsExternalLabel;
  }

private:
  std::vector<std::byte> storage;
  std::vector<std::pair<std::shared_ptr<inst::Instruction>, std::size_t>>
      instsAndPos;
  std::unordered_map<std::string, std::size_t> symbolTable;
  std::unordered_set<std::string> globalSymbol;
  std::unordered_map<inst::Instruction::Id, std::string> containsExternalLabel;
  std::unordered_map<inst::Instruction::Id, std::string> containsRelocationFunc;
};

} // namespace ravel
