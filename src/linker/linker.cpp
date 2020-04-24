#include "linker.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "assembler/assembler.h"
#include "assembler/parser.h"
#include "container_utils.h"
#include "error.h"

namespace ravel {
namespace {
class SymbolTable {
public:
  std::size_t resolve(const std::string &symbol) const {
    auto opt = get(globalSymTable, symbol);
    if (opt)
      return opt.value();
    throw UnresolvableSymbol(symbol);
  }

  std::size_t resolve(ObjectFile::Id objId, const std::string &symbol) const {
    auto opt = get(objSymTables.at(objId), symbol);
    if (opt)
      return opt.value();
    return resolve(symbol);
  }

  void addGlobalSymbol(const std::string &sym, std::size_t pos) {
    if (isIn(globalSymTable, sym))
      throw DuplicatedSymbols(sym);
    globalSymTable.emplace(sym, pos);
  }

  void
  addObjSymTable(ObjectFile::Id objId, std::size_t basePos,
                 const std::unordered_map<std::string, std::size_t> &symTable,
                 const std::unordered_set<std::string> &globalSymbols) {
    std::unordered_map<std::string, std::size_t> newTable;
    for (auto &[sym, pos] : symTable) {
      newTable.emplace(sym, basePos + pos);
      if (isIn(globalSymbols, sym))
        addGlobalSymbol(sym, basePos + pos);
    }
    objSymTables.emplace(objId, newTable);
  }

private:
  std::unordered_map<ObjectFile::Id,
                     std::unordered_map<std::string, std::size_t>>
      objSymTables;
  std::unordered_map<std::string, std::size_t> globalSymTable;
};

// We need to merge the object files into one interpretable file, resolve
// external symbols and compute relocation functions. For simplicity, we shall
// identify an object file by its position in `objects`.
//
// First, we allocate spaces and build the symbol table. Also, we record the
// starting position of each object file. Note that we add an extra object file
// containing `_start` and some placeholders for libc functions.
// (cf. `void Linker::prepare()`)
//
// Then, we merge the object files. Namely, we merge the `storage`s and
// instructions of the object files and encode the instructions in the right
// way. We will record from which object file each instruction comes from, and
// a mapping from positions to `pcrel_hi` (if there is one). These will be
// helpful in the next phase.
// (cf. `void mergeObjects(const ObjectFile & obj)`)
//
// After that, we resolve global symbols and compute the relocation functions.
// These will be done in a single pass since we will replace instructions in
// `insts`, whence change the instruction ID.
// (cf. `void handleRelocationFuncsAndExternalSymbols()`)
//
// Finally, we handle directives such as `.word symbol`.
class Linker {
public:
  explicit Linker(const std::vector<ObjectFile> &objectsList) {
    for (const auto &obj : objectsList)
      objects.emplace(obj.getId(), obj);
  }

  Interpretable link() {
    prepare();
    for (auto &[_, obj] : objects)
      mergeObject(obj);
    handleRelocationFuncsAndExternalSymbols();

    for (auto &[objId, obj] : objects) {
      auto startPos = startingPosition.at(objId);
      for (auto [label, pos] : obj.getToBeStored()) {
        std::uint32_t addr = symTable.resolve(objId, label);
        pos += startPos;
        *(std::uint32_t *)(storage.data() + pos) = addr;
      }
    }

    return {storage, insts};
  }

private:
  static ObjectFile makeStartObj() {
    std::string src = ".text\n_start:\ncall main\nnop";
    for (int i = 0; i < 48 - (3 * 4); i += 4) {
      src += "\nnop";
    }
    auto res = assemble(src);
    assert(res.getStorage().size() == 48);
    return res;
  }

  void prepare() {
    auto startObj = makeStartObj();
    objects.emplace(startObj.getId(), startObj);
    for (const auto &[name, pos] : libc::getName2Pos()) {
      symTable.addGlobalSymbol(name, pos);
    }

    for (auto &[id, obj] : objects) {
      std::size_t basePos = storage.size();
      startingPosition[id] = basePos;
      symTable.addObjSymTable(id, basePos, obj.getSymbolTable(),
                              obj.getGlobalSymbol());
      storage.resize(storage.size() + obj.getStorage().size());
    }
  }

  void mergeObject(const ObjectFile &obj) {
    auto basePos = startingPosition.at(obj.getId());
    std::copy(obj.getStorage().begin(), obj.getStorage().end(),
              storage.begin() + basePos);
    for (const auto &inst : obj.getInsts()) {
      auto pos = basePos + obj.getInst2Pos().at(inst->getId());
      assert(pos + 3 < storage.size());
      *(std::uint32_t *)(storage.data() + pos) = insts.size();
      insts.emplace_back(inst);
      inst2ObjId.emplace(inst->getId(), obj.getId());
      if (auto opt = get(obj.getContainsRelocationFunc(), inst->getId());
          opt && opt.value().type == RelocationFunction::PCREL_HI) {
        pos2Relocation.emplace(pos, opt.value());
      }
    }
  }

  void handleRelocationFuncsAndExternalSymbols() {
    for (auto &inst : insts) {
      auto instId = inst->getId(); // make a copy of the instruction ID
      auto objId = inst2ObjId.at(instId);
      const auto &obj = objects.at(objId);
      if (isIn(obj.getContainsExternalLabel(), instId)) {
        assert(false);
        // TODO: Does external label outsides relocation function really exist?
      }
      if (isIn(obj.getContainsRelocationFunc(), instId)) {
        // `inst` may have been replaced in the previous stage, so pass in the
        // instruction ID explicitly
        inst = computeRelocationFunction(inst, instId, obj);
      }
    }
  }

  std::shared_ptr<inst::Instruction>
  computeRelocationFunction(const std::shared_ptr<inst::Instruction> &inst,
                            inst::Instruction::Id instId,
                            const ObjectFile &obj) const {
    auto relocation = obj.getContainsRelocationFunc().at(instId);
    auto imm =
        symTable.resolve(obj.getId(), relocation.symbol) + relocation.offset;
    if (relocation.type == RelocationFunction::HI) {
      return replaceImm(inst, imm >> 12u);
    }
    if (relocation.type == RelocationFunction::LO) {
      return replaceImm(inst, imm & 0xfffu);
    }
    auto basePos = startingPosition.at(obj.getId());
    if (relocation.type == RelocationFunction::PCREL_HI) {
      auto instPos = basePos + obj.getInst2Pos().at(instId);
      auto offset = imm - instPos;
      return replaceImm(inst, offset >> 12u);
    }
    assert(relocation.type == RelocationFunction::PCREL_LO);
    auto pcrelHiPos = imm;
    auto pcrelHi = pos2Relocation.at(pcrelHiPos);
    auto pcrelHiImm =
        symTable.resolve(obj.getId(), pcrelHi.symbol) + pcrelHi.offset;
    auto offset = pcrelHiImm - pcrelHiPos;
    return replaceImm(inst, offset & 0xfffu);
  }

  static std::shared_ptr<inst::Instruction>
  replaceImm(const std::shared_ptr<inst::Instruction> &inst,
             std::uint32_t imm) {
    if (auto p = std::dynamic_pointer_cast<inst::ImmConstruction>(inst)) {
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
    if (auto p = std::dynamic_pointer_cast<inst::JumpLinkReg>(inst)) {
      return std::make_shared<inst::JumpLinkReg>(p->getDest(), p->getBase(),
                                                 imm);
    }
    assert(false);
  }

private:
  std::unordered_map<ObjectFile::Id, ObjectFile> objects;
  std::unordered_map<ObjectFile::Id, std::size_t> startingPosition;
  SymbolTable symTable;
  std::unordered_map<inst::Instruction::Id, ObjectFile::Id> inst2ObjId;
  std::unordered_map<std::size_t, RelocationFunction> pos2Relocation;

  std::vector<std::byte> storage;
  std::vector<std::shared_ptr<inst::Instruction>> insts;
};

} // namespace

} // namespace ravel

namespace ravel {

Interpretable link(const std::vector<ObjectFile> &objects) {
  Linker linker(objects);
  return linker.link();
}

} // namespace ravel
