#include "linker.h"

#include <cassert>
#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "assembler/assembler.h"
#include "assembler/parser.h"
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
    // Relocation functions are computed during the merging phrase if possible.
    // Those ones containing global symbols will be computed in the symbol
    // resolving phrase.

    mergeObj(makeStartObj());
    storage.resize(48);
    for (auto &[name, pos] : libc::getName2Pos()) {
      symbolTable.emplace(name, pos);
    }
    for (auto &obj : objects) {
      mergeObj(obj);
    }
    for (auto &[id, _] : containsRelocationFunc)
      assert(!isIn(containsUnresolvedSymbol, id));
    computeGlobalRelocationFuncs();
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
      if (!isIn(obj.getContainsRelocationFunc(), inst->getId())) {
        instsAndPos.emplace_back(inst, storage.size() + pos);
        continue;
      }
      std::string relocationFunc =
          obj.getContainsRelocationFunc().at(inst->getId());
      if (auto immOpt =
              computeRelocationFunc(relocationFunc, obj.getSymbolTable())) {
        inst = replaceImm(inst, immOpt.value());
        instsAndPos.emplace_back(inst, storage.size() + pos);
        continue;
      }
      containsRelocationFunc.emplace(inst->getId(), relocationFunc);
    }

    // merge the symbol table
    for (auto &[label, pos] : obj.getSymbolTable()) {
      if (!isIn(obj.getGlobalSymbol(), label)) {
        continue;
      }
      if (isIn(symbolTable, label)) {
        throw DuplicatedSymbols(label);
      }
      symbolTable.emplace(label, storage.size() + pos);
    }

    containsUnresolvedSymbol.insert(obj.getContainsExternalLabel().begin(),
                                    obj.getContainsExternalLabel().end());
    append(storage, objStorage);
  }

  std::optional<std::uint32_t> computeRelocationFunc(
      const std::string &str,
      const std::unordered_map<std::string, std::size_t> &objSymTable) {
    auto hiLo = str.substr(0, 3);
    assert(hiLo == "%hi" || hiLo == "%lo");
    auto getHiLo = [&hiLo](std::uint32_t imm) {
      if (hiLo == "%hi")
        return imm >> 12;
      assert(hiLo == "%lo");
      return imm & 0xfff;
    };

    auto exprStr = str.substr(4);
    exprStr.pop_back();
    auto tokens = split(exprStr, "+");
    if (tokens.size() == 1) {
      if (!isIn(objSymTable, exprStr))
        return std::nullopt;
      return {getHiLo(storage.size() + objSymTable.at(exprStr))};
    }
    assert(tokens.size() == 2);
    // e.g. foo+4
    if (!isIn(objSymTable, tokens.front()))
      return std::nullopt;
    auto pos = objSymTable.at(tokens.front());
    int offset = std::stoi(tokens.at(1), nullptr, 0);
    return {getHiLo(storage.size() + pos + offset)};
  }

  static std::shared_ptr<inst::Instruction>
  replaceImm(const std::shared_ptr<inst::Instruction> &inst,
             std::uint32_t imm) {
    if (auto p = std::dynamic_pointer_cast<inst::ImmConstruction>(inst)) {
      assert(inst->getOp() == inst::Instruction::LUI);
      return std::make_shared<inst::ImmConstruction>(p->getOp(), p->getDest(),
                                                     imm);
    }
    if (auto p = std::dynamic_pointer_cast<inst::ArithRegImm>(inst)) {
      return std::make_shared<inst::ArithRegImm>(p->getOp(), p->getDest(),
                                                 p->getSrc(), imm);
    }
    if (auto p = std::dynamic_pointer_cast<inst::MemAccess>(inst)) {
      return std::make_shared<inst::MemAccess>(p->getOp(), p->getReg(),
                                               p->getBase(), imm);
    }
    assert(false);
  }

  void computeGlobalRelocationFuncs() {
    for (auto &[inst, pos] : instsAndPos) {
      if (!isIn(containsRelocationFunc, inst->getId()))
        continue;
      std::string relocationFunc = containsRelocationFunc.at(inst->getId());
      auto imm = computeRelocationFunc(relocationFunc, symbolTable);
      assert(imm);
      inst = replaceImm(inst, imm.value());
    }
  }

  void resolveSymbols() {
    for (auto &[inst, pos] : instsAndPos) {
      if (!isIn(containsUnresolvedSymbol, inst->getId()))
        continue;
      auto sym = containsUnresolvedSymbol.at(inst->getId());
      auto symPos = symbolTable.at(sym);
      int offset = symPos - pos;
      if (inst->getOp() == inst::Instruction::AUIPC) {
        auto auipc = std::dynamic_pointer_cast<inst::ImmConstruction>(inst);
        assert(auipc);
        inst = std::make_shared<inst::ImmConstruction>(
            inst::Instruction::AUIPC, auipc->getDest(),
            std::uint32_t(offset) >> 12);
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
  std::unordered_map<inst::Instruction::Id, std::string> containsRelocationFunc;
};

} // namespace

} // namespace ravel

namespace ravel {

Interpretable link(const std::vector<ObjectFile> &objects) {
  Linker linker(objects);
  return linker.link();
}

} // namespace ravel
