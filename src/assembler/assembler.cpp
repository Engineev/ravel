#include "assembler.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "container_utils.h"
#include "error.h"
#include "instructions.h"
#include "object_file.h"
#include "parser.h"

// Here, we describe the implementation of the assembler. For details about
// the resulted [ObjectFile], see object_file.h.
//     We assume each line contains either a directive, a label, or and
// instruction. See the function [preprocess] in parser.cpp for detail on
// preprocessing.
//     The assembling process consists of two passes. In the first pass,
// spaces are allocated, and labels and directives are handled. In the second
// pass, instructions are parsed.
// TODO:
//   * local labels (numerical labels) are not supported yet

namespace ravel {
struct LabelName {
  LabelName() = default;
  LabelName(std::string global, std::string local)
      : global(std::move(global)), local(std::move(local)) {}

  std::string global, local;

  bool operator==(const LabelName &rhs) const {
    return global == rhs.global && local == rhs.local;
  }

  bool operator!=(const LabelName &rhs) const { return !(*this == rhs); }
};

enum class Section { ERROR = 0, TEXT, DATA, RODATA, BSS };

struct LabelPos {
  LabelPos(Section section, size_t pos) : section(section), pos(pos) {}

  Section section;
  std::size_t pos;
};

int roundUp(int number, int multiple) {
  assert(multiple > 0);
  int remainder = number % multiple;
  if (remainder == 0)
    return number;
  return number + multiple - remainder;
}

} // namespace ravel

namespace std {
template <> struct hash<ravel::LabelName> {
  using Key = ravel::LabelName;

  std::size_t operator()(const Key &k) const {
    return std::hash<std::string>()(k.global + "." + k.local);
  }
};
} // namespace std

namespace ravel {
namespace {

class AssemblerPass1 {
public:
  explicit AssemblerPass1(std::vector<std::string> src) : src(std::move(src)) {}

  std::tuple<std::vector<std::byte> /* storage */,
             std::unordered_map<LabelName, std::size_t> /* labelName2Pos */,
             std::unordered_set<std::string> /* globalSymbols */>
  operator()() {
    for (auto &line : src) {
      if (isDirective(line)) {
        handleDerivative(line);
        continue;
      }
      if (isLabel(line)) {
        handleLabel(line);
        continue;
      }
      assert(curSection == Section::TEXT);
      handleInstruction(line);
    }

    // merge the result
    text.resize(roundUp(text.size(), 16));
    data.resize(roundUp(data.size(), 16));
    rodata.resize(roundUp(rodata.size(), 16));
    bss.resize(roundUp(bss.size(), 16));

    std::vector<std::byte> storage;
    storage.insert(storage.end(), text.begin(), text.end());
    storage.insert(storage.end(), data.begin(), data.end());
    storage.insert(storage.end(), rodata.begin(), rodata.end());
    storage.insert(storage.end(), bss.begin(), bss.end());

    std::unordered_map<LabelName, std::size_t> labelName2Pos;

    for (auto &[labelName, secPos] : labelName2SecPos) {
      std::size_t pos = 0;
      switch (secPos.section) { // The fall-through is intended here.
      case Section::BSS:
        pos += rodata.size();
      case Section::RODATA:
        pos += data.size();
      case Section::DATA:
        pos += text.size();
      case Section::TEXT:
        pos += secPos.pos;
        break;
      default:
        assert(false);
      }
      labelName2Pos.emplace(labelName, pos);
    }

    return {storage, labelName2Pos, globalSymbols};
  }

private:
  std::vector<std::byte> &getCurSecStorage() {
    switch (curSection) {
    case Section::TEXT:
      return text;
    case Section::DATA:
      return data;
    case Section::RODATA:
      return rodata;
    case Section::BSS:
      return bss;
    default:
      assert(false);
    }
  }

  void handleDerivative(const std::string &line) {
    auto tokens = tokenize(line);
    assert(!tokens.empty());

    if (tokens[0] == ".text") {
      curSection = Section::TEXT;
      return;
    }
    if (tokens[0] == ".data") {
      curSection = Section::DATA;
      return;
    }
    if (tokens[0] == ".rodata") {
      curSection = Section::RODATA;
      return;
    }
    if (tokens[0] == ".bss") {
      curSection = Section::BSS;
      return;
    }
    if (tokens[0] == ".section") {
      auto sec = parseSectionDerivative(line);
      if (sec == ".text")
        curSection = Section ::TEXT;
      else if (sec == ".data")
        curSection = Section ::DATA;
      else if (sec == ".rodata")
        curSection = Section ::RODATA;
      else if (sec == ".bss")
        curSection = Section ::BSS;
      else {
        std::cerr << "Ignoring directive: " << line << std::endl;
      }
      return;
    }
    if (curSection == Section::ERROR) {
      std::cerr << "Ignoring directive: " << line << std::endl;
      return;
    }

    auto &storage = getCurSecStorage();

    if (tokens[0] == ".align" || tokens[0] == ".p2align") {
      auto p = std::stoul(tokens.at(1));
      auto alignment = 1 << p;
      storage.resize(roundUp(storage.size(), alignment));
      return;
    }
    if (tokens[0] == ".globl") {
      globalSymbols.emplace(tokens.at(1));
      return;
    }
    if (tokens[0] == ".comm") {
      auto label = tokens.at(1);
      std::size_t size = std::stoul(tokens.at(2));
      std::size_t alignment = std::stoul(tokens.at(3));
      bss.resize(roundUp(bss.size(), alignment));
      auto pos = bss.size();
      bss.resize(bss.size() + size);
      labelName2SecPos.emplace(LabelName(label, ""),
                               LabelPos(Section::BSS, pos));
      return;
    }
    if (tokens[0] == ".zero") {
      std::size_t size = std::stoul(tokens.at(1));
      storage.resize(storage.size() + size, (std::byte)0);
      return;
    }

    if (tokens.at(0) == ".string" || tokens.at(0) == ".asciz") {
      assert(curSection != Section::TEXT);
      auto iter = line.begin();
      iter += tokens[0].length();
      while (std::isspace(*iter))
        ++iter;
      assert(*iter == '"');
      ++iter;
      std::string str(iter, line.end() - 1);
      str = handleEscapeCharacters(str);
      auto curPos = storage.size();
      storage.resize(storage.size() + str.size() + 1);
      std::strcpy((char *)(storage.data() + curPos), str.c_str());
      storage.back() = (std::byte)0;
      return;
    }
    if (tokens.at(0) == ".word") {
      assert(curSection != Section::TEXT);
      std::int32_t val = std::stoi(tokens.at(1), nullptr, 0);
      auto curPos = storage.size();
      storage.resize(storage.size() + 4);
      *(std::int32_t *)(storage.data() + curPos) = val;
      return;
    }

    std::cerr << "Ignoring directive: " << line << std::endl;
  }

  void handleLabel(std::string label) {
    assert(tokenize(label).size() == 1);
    label.pop_back(); // remove ':'
    labelName2SecPos.emplace(LabelName(label, ""),
                             LabelPos(curSection, getCurSecStorage().size()));
  }

  void handleInstruction(const std::string &line) {
    auto tokens = tokenize(line);
    std::size_t nInsts = (tokens.at(0) == "call" || tokens.at(0) == "tail" ||
                          tokens.at(0) == "li")
                             ? 2
                             : 1;
    text.resize(text.size() + 4 * nInsts);
  }

private:
  std::vector<std::string> src;
  Section curSection = Section::ERROR;

  std::vector<std::byte> text, data, rodata, bss;
  std::unordered_set<std::string> globalSymbols;
  std::unordered_map<LabelName, LabelPos> labelName2SecPos;
};

class AssemblerPass2 {
public:
  AssemblerPass2(
      const std::vector<std::string> &src,
      const std::unordered_map<LabelName, std::size_t> &labelName2Pos)
      : src(src), labelName2Pos(std::move(labelName2Pos)) {}

  auto operator()() {
    bool isText = false;
    for (auto &line : src) {
      auto tokens = tokenize(line);

      if (line == ".text") {
        isText = true;
        continue;
      }
      if (line == ".data" || line == ".rodata" || line == ".bss") {
        isText = false;
        continue;
      }
      if (tokens.at(0) == ".section") {
        auto secName = parseSectionDerivative(line);
        isText = secName == ".text";
        continue;
      }
      if (!isText)
        continue;
      parseCurrentLine(line);
    }

    return std::make_tuple(instsAndPos, containsExternalLabel,
                           containsRelocationFunc);
  }

private:
  void parseCurrentLine(const std::string &line) {
    auto tokens = tokenize(line);
    if (isDirective(line)) {
      if (tokens.at(0) != ".align")
        return;
      auto alignment = std::stoul(tokens.at(1));
      curPos = roundUp(curPos, alignment);
      return;
    }
    if (isLabel(line)) {
      assert(tokens.size() == 1);
      return;
    }
    handleInstruction(line);
  }

  void handleInstruction(const std::string &line) {
    auto tokens = tokenize(line);
    assert(!tokens.empty());

    // handle pseudo instructions
    if (tokens[0] == "call" || tokens[0] == "tail") {
      handleCall(tokens);
      return;
    }
    if (tokens[0] == "li") {
      handleLi(tokens);
      return;
    }

    handleNonPseudoInst(translatePseudoInst(line));
  }

  // If [str] is a number, then return it.
  // If [str] is a non-external label, then compute the offset if possible
  std::optional<int> getOffset(std::string str) const {
    if (std::isdigit(str.front())) {
      return std::stoi(str, nullptr, 0);
    }
    // is a label
    assert(!std::isdigit(str.front()) &&
           "local label has not been supported yet");
    auto pos = get(labelName2Pos, LabelName(str, ""));
    if (!pos)
      return std::nullopt;
    return (std::int64_t)pos.value() - (std::int64_t)curPos;
  }

  void handleLi(const std::vector<std::string> &tokens) {
    auto dest = tokens.at(1);
    auto imm = std::stoi(tokens.at(2), nullptr, 0);
    auto uImm = std::uint32_t(imm) >> 12;
    auto lImm = std::uint32_t(imm) & 0xfff;
    auto lui = "lui " + dest + ", " + std::to_string(uImm);
    auto ori = "ori " + dest + ", " + dest + ", " + std::to_string(lImm);
    handleNonPseudoInst(lui);
    handleNonPseudoInst(ori);
  }

  void handleCall(const std::vector<std::string> &tokens) {
    using namespace std::string_literals;
    assert(tokens.size() == 2);
    assert(tokens[0] == "call" || tokens[0] == "tail");
    std::string auipc, jalr;
    auipc = "auipc x6, ";
    jalr = "jalr "s + (tokens[0] == "call" ? "x1"s : "x0"s) + ", ";

    auto funcName = tokens.at(1);
    auto offsetOpt = getOffset(funcName);
    if (!offsetOpt) { // an external function
      auipc += "0";
      jalr += "0(x6)";
      handleNonPseudoInst(auipc);
      containsExternalLabel.emplace(instsAndPos.back().first->getId(),
                                    funcName);
      handleNonPseudoInst(jalr);
      containsExternalLabel.emplace(instsAndPos.back().first->getId(),
                                    funcName);
      return;
    }
    // an interval function
    auto offset = offsetOpt.value();
    auipc += std::to_string(offset >> 12);
    jalr += std::to_string(offset & 0xfff) + "(x6)";
    handleNonPseudoInst(auipc);
    handleNonPseudoInst(jalr);
    instsAndPos.back().first->setComment(funcName);
  }

  void handleNonPseudoInst(const std::string &line) {
    auto tokens = tokenize(line);
    auto inst = parseInst(tokens);
    instsAndPos.emplace_back(inst, curPos);
    curPos += 4;
  }

  std::shared_ptr<inst::Instruction>
  parseInst(const std::vector<std::string> &tokens) {
    // const std::size_t DummyOffset = 0;
    assert(!tokens.empty());

    inst::Instruction::OpType op;
    try {
      op = name2OpType(tokens[0]);
    } catch (std::out_of_range &e) {
      throw NotSupportedError("Unknown op: " + tokens[0]);
    }

    if (op == inst::Instruction::LUI || op == inst::Instruction::AUIPC) {
      auto dest = regName2regNumber(tokens.at(1));
      auto imm = parseImm(tokens.at(2));
      auto inst =
          std::make_shared<inst::ImmConstruction>(op, dest, imm.value_or(0));
      if (!imm)
        containsRelocationFunc.emplace(inst->getId(), tokens.at(2));
      return inst;
    }

    static std::unordered_set<std::string> arithRegReg = {
        "add", "sub", "sll", "slt", "sltu", "xor", "srl", "sra", "or", "and"};
    if (isIn(arithRegReg, tokens[0])) {
      auto dest = regName2regNumber(tokens.at(1));
      auto src1 = regName2regNumber(tokens.at(2));
      auto src2 = regName2regNumber(tokens.at(3));
      return std::make_shared<inst::ArithRegReg>(op, dest, src1, src2);
    }

    static std::unordered_set<std::string> arithRegImmInsts = {
        "addi", "slti", "sltiu", "xori", "ori", "andi", "slli", "srli", "srai"};
    if (isIn(arithRegImmInsts, tokens.at(0))) {
      auto dest = regName2regNumber(tokens.at(1));
      auto rc = regName2regNumber(tokens.at(2));
      auto imm = parseImm(tokens.at(3));
      auto inst =
          std::make_shared<inst::ArithRegImm>(op, dest, rc, imm.value_or(0));
      if (!imm)
        containsRelocationFunc.emplace(inst->getId(), tokens.at(3));
      return inst;
    }

    static std::unordered_set<std::string> memAccessInsts = {
        "lb", "lh", "lw", "lbu", "lhu", "sb", "sh", "sw"};
    if (isIn(memAccessInsts, tokens[0])) {
      auto reg = regName2regNumber(tokens.at(1));
      if (tokens.at(2).front() == '%') { // e.g. %lo(l)(a5)
        auto addrTokens = split(tokens[2], "()");
        assert(addrTokens.at(0) == "%lo");
        auto relocation = "%lo(" + addrTokens.at(1) + ")";
        auto base = regName2regNumber(addrTokens.back());
        auto inst = std::make_shared<inst::MemAccess>(op, reg, base, 0);
        containsRelocationFunc.emplace(inst->getId(), relocation);
        return inst;
      }

      auto [base, offset] = parseBaseOffset(tokens.at(2));
      return std::make_shared<inst::MemAccess>(op, reg, base, offset);
    }

    if (op == inst::Instruction::JAL) {
      auto dest = regName2regNumber(tokens.at(1));
      auto offsetOpt = getOffset(tokens.at(2));
      if (offsetOpt)
        return std::make_shared<inst::JumpLink>(dest, offsetOpt.value() / 2,
                                                tokens.at(2));
      auto inst = std::make_shared<inst::JumpLink>(dest, 0, tokens[2]);
      containsExternalLabel.emplace(inst->getId(), tokens[2]);
      return inst;
    }

    if (op == inst::Instruction::JALR) {
      auto dest = regName2regNumber(tokens.at(1));
      auto [base, offset] = parseBaseOffset(tokens.at(2));
      return std::make_shared<inst::JumpLinkReg>(dest, base, offset);
    }

    static std::unordered_set<std::string> branchInsts = {
        "beq", "bne", "blt", "bge", "bltu", "bgeu"};
    if (isIn(branchInsts, tokens[0])) {
      auto src1 = regName2regNumber(tokens.at(1));
      auto src2 = regName2regNumber(tokens.at(2));
      auto offsetOpt = getOffset(tokens.at(3));
      if (offsetOpt)
        return std::make_shared<inst::Branch>(op, src1, src2, offsetOpt.value(),
                                              tokens[3]);
      auto inst = std::make_shared<inst::Branch>(op, src1, src2, 0, tokens[3]);
      containsExternalLabel.emplace(inst->getId(), tokens.at(3));
      return inst;
    }

    static std::unordered_set<std::string> mArithInsts = {
        "mul", "mulh", "mulhsu", "mulhu", "div", "divu", "rem", "rem"};
    if (isIn(mArithInsts, tokens[0])) {
      auto dest = regName2regNumber(tokens.at(1));
      auto src1 = regName2regNumber(tokens.at(2));
      auto src2 = regName2regNumber(tokens.at(3));
      return std::make_shared<inst::MArith>(op, dest, src1, src2);
    }

    assert(false);
  }

private:
  const std::vector<std::string> &src;
  std::size_t curPos = 0;

  std::vector<std::pair<std::shared_ptr<inst::Instruction>, std::size_t>>
      instsAndPos;
  const std::unordered_map<LabelName, std::size_t> &labelName2Pos;
  // When we encounter an instruction which contains an external label, then
  // we add the instruction ID and the label name into this.
  std::unordered_map<inst::Instruction::Id, std::string> containsExternalLabel;
  std::unordered_map<inst::Instruction::Id, std::string> containsRelocationFunc;
};

} // namespace
} // namespace ravel

namespace ravel {

ObjectFile assemble(const std::string &src) {
  auto lines = preprocess(src);
  auto [storage, labelName2Pos, globalSymbols] = AssemblerPass1(lines)();
  auto [instsAndPos, containsExternalLabel, containsRelocationFunc] =
      AssemblerPass2(lines, labelName2Pos)();

  std::unordered_map<std::string, std::size_t> symTable;
  for (auto &[label, pos] : labelName2Pos)
    symTable.emplace(label.global, pos);
  return {std::move(storage), std::move(instsAndPos), std::move(symTable),
          globalSymbols,      containsExternalLabel,  containsRelocationFunc};
}

} // namespace ravel