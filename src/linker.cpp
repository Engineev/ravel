#include "linker.h"

#include <cassert>
#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "assembler.h"
#include "container_utils.h"

namespace ravel {
namespace {

class Linker {
public:
  explicit Linker(const std::vector<ObjectFile> &objects) : objects(objects) {}

  Interpretable link() {
    // We need to merge the object files into one interpretable file and
    // for each object file, resolve the external symbols.
    // First, we merge the [storage]'s and instructions. Recall that in
    // [storage], instructions are encoded as the indices in [insts]. Hence,
    // we need to store the index of each instruction to the corresponding
    // position of [storage] on the fly.
    // Meanwhile, in order to resolve unresolved symbols, for each global
    // symbol defined in each object file, we need to record its current
    // position in [storage]. The ID of instructions containing unresolved
    // symbols should also be recorded.

    mergeObj(makeStartObj());
    storage.resize(48);
    for (auto &[name, pos] : libc::getName2Pos()) {
      symbolTable.emplace(name, pos);
    }
    for (auto &obj : objects) {
      mergeObj(obj);
    }
    resolveSymbols();

    std::vector<std::shared_ptr<inst::Instruction>> insts;
    for (auto &[inst, _] : instsAndPos)
      insts.emplace_back(inst);
    return {std::move(storage), insts};
  }

private:
  static ObjectFile makeStartObj() {
    std::string src = ".text\n_start:\ncall main\nnop";
    return assemble(src);
  }

  void mergeObj(const ObjectFile &obj) {
    auto objStorage = obj.getStorage();
    auto &objInstsAndPos = obj.getInstsAndPos();

    // place the instructions into [objStorage] and build symbol table
    for (auto [inst, pos] : objInstsAndPos) {
      *(std::uint32_t *)(objStorage.data() + pos) = instsAndPos.size();
      if (isIn(obj.getContainsRelocationFunc(), inst->getId())) {
        inst = computeRelocatedImm(inst);
      }
      instsAndPos.emplace_back(inst, storage.size() + pos);
    }
    for (auto [label, pos] : obj.getGlobalLabel2Pos()) {
      if (isIn(symbolTable, label)) {
        throw DuplicatedSymbols(label);
      }
      symbolTable.emplace(label, storage.size() + pos);
    }

    containsUnresolvedSymbol.insert(obj.getContainsExternalLabel().begin(),
                                    obj.getContainsExternalLabel().end());
    append(storage, objStorage);
  }

  std::shared_ptr<inst::Instruction>
  computeRelocatedImm(const std::shared_ptr<inst::Instruction> &inst) const {
    if (inst->getOp() == inst::Instruction::LUI) {
      auto p = std::static_pointer_cast<inst::ImmConstruction>(inst);
      auto imm = ((p->getImm() << 12) + storage.size()) >> 12;
      return std::make_shared<inst::ImmConstruction>(inst::Instruction::LUI,
                                                     p->getDest(), imm);
    }
    if (inst->getOp() == inst::Instruction::ADDI) {
      auto p = std::static_pointer_cast<inst::ArithRegImm>(inst);
      auto imm = (p->getImm() + storage.size()) & 0xfff;
      return std::make_shared<inst::ArithRegImm>(
          inst::Instruction::ADDI, p->getDest(), p->getSrc(), imm);
    }
    assert(false);
  }

  void resolveSymbols() {
    for (auto &[inst, pos] : instsAndPos) {
      if (!isIn(containsUnresolvedSymbol, inst->getId()))
        continue;
      auto symPos = symbolTable.at(containsUnresolvedSymbol.at(inst->getId()));
      int offset = symPos - pos;
      if (inst->getOp() == inst::Instruction::AUIPC) {
        auto auipc = std::dynamic_pointer_cast<inst::ImmConstruction>(inst);
        assert(auipc);
        inst = std::make_shared<inst::ImmConstruction>(
            inst::Instruction::AUIPC, auipc->getDest(), offset >> 12);
        continue;
      }
      if (inst->getOp() == inst::Instruction::JALR) {
        auto jalr = std::dynamic_pointer_cast<inst::JumpLinkReg>(inst);
        assert(jalr);
        inst = std::make_shared<inst::JumpLinkReg>(
            jalr->getDest(), jalr->getBase(), (offset + 4) & 0xfff);
        continue;
      }
      assert(false); // TODO
    }
  }

  std::size_t resolveSymbol(const std::string &label) const {
    return symbolTable.at(label);
  }

private:
  const std::vector<ObjectFile> &objects;

  std::vector<std::byte> storage;
  std::vector<std::pair<std::shared_ptr<inst::Instruction>, std::size_t>>
      instsAndPos;
  std::unordered_map<std::string, std::size_t> symbolTable;
  std::unordered_map<inst::Instruction::Id, std::string>
      containsUnresolvedSymbol;
};

} // namespace

} // namespace ravel

namespace ravel {

Interpretable link(const std::vector<ObjectFile> &objects) {
  Linker linker(objects);
  return linker.link();
}

} // namespace ravel
