#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "container_utils.h"
#include "instructions.h"

namespace ravel {

struct RelocationFunction {
  enum Type {
    HI,
    LO,
    PCREL_HI,
    PCREL_LO,
  };

  RelocationFunction(Type type, std::string symbol, int offset = 0)
      : type(type), symbol(std::move(symbol)), offset(offset) {}

  Type type;
  std::string symbol;
  int offset;
};

} // namespace ravel

namespace ravel {
// `ObjectFile` contains the following fields:
// * `std::vector<std::byte> storage`: the content of the object file, including
//     initialized and uninitialized data. Note that instructions are encoded in
//     by their positions in `instructions`.
// * std::unordered_map<std::string, std::size_t> symbolTable
// * std::unordered_set<std::string> globalSymbol
// * std::unordered_map<inst::Instruction::Id, std::string>
//     containsExternalLabel: In the object file, some instructions still
//     contain external labels which are going to be resolved in the linking
//     phrase. This map contains their ID and the name of label.
// * std::unordered_map<inst::Instruction::Id,
//     std::shared_ptr<RelocationFunction>> containsRelocationFunc:
//     Some instructions contains relocation functions such as %hi and this map
//     contains them and the relocation functions.
//
// Layout: text, data, rodata, bss
class ObjectFile {
public:
  using Id = ravel::Id<ObjectFile>;

  ObjectFile(std::vector<std::byte> storage,
             std::vector<std::shared_ptr<inst::Instruction>> insts,
             std::unordered_map<inst::Instruction::Id, std::size_t> inst2Pos,
             std::unordered_map<std::string, std::size_t> symbolTable,
             std::unordered_set<std::string> globalSymbol,
             std::unordered_map<inst::Instruction::Id, std::string>
                 containsExternalLabel,
             std::unordered_map<inst::Instruction::Id, RelocationFunction>
                 containsRelocationFunc,
             std::vector<std::pair<std::string, std::size_t>> toBeStored)
      : storage(std::move(storage)), insts(std::move(insts)),
        inst2Pos(std::move(inst2Pos)), symbolTable(std::move(symbolTable)),
        globalSymbol(std::move(globalSymbol)),
        containsExternalLabel(std::move(containsExternalLabel)),
        containsRelocationFunc(std::move(containsRelocationFunc)),
        toBeStored(std::move(toBeStored)) {}

  const Id &getId() const { return id; }

  const std::vector<std::byte> &getStorage() const { return storage; }

  const std::vector<std::shared_ptr<inst::Instruction>> &getInsts() const {
    return insts;
  }

  const std::unordered_map<inst::Instruction::Id, std::size_t> &
  getInst2Pos() const {
    return inst2Pos;
  }

  const std::unordered_map<std::string, std::size_t> &getSymbolTable() const {
    return symbolTable;
  }

  const std::unordered_set<std::string> &getGlobalSymbol() const {
    return globalSymbol;
  }

  const std::unordered_map<inst::Instruction::Id, std::string> &
  getContainsExternalLabel() const {
    return containsExternalLabel;
  }

  const std::unordered_map<inst::Instruction::Id, RelocationFunction> &
  getContainsRelocationFunc() const {
    return containsRelocationFunc;
  }

  const std::vector<std::pair<std::string, std::size_t>> &
  getToBeStored() const {
    return toBeStored;
  }

private:
  Id id;
  std::vector<std::byte> storage;
  std::vector<std::shared_ptr<inst::Instruction>> insts;
  std::unordered_map<inst::Instruction::Id, std::size_t> inst2Pos;

  std::unordered_map<std::string, std::size_t> symbolTable;
  std::unordered_set<std::string> globalSymbol;
  std::unordered_map<inst::Instruction::Id, std::string> containsExternalLabel;
  std::unordered_map<inst::Instruction::Id, RelocationFunction>
      containsRelocationFunc;
  std::vector<std::pair<std::string, std::size_t>> toBeStored;
};

} // namespace ravel
