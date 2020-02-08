#include "assembler.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "container_utils.h"
#include "instructions.h"
#include "object_file.h"
#include "parser.h"

namespace ravel {
namespace {
struct LabelName {
  LabelName() = default;
  LabelName(std::string global, std::string local)
      : global(global), local(local) {}

  std::string global, local;

  bool operator==(const LabelName &rhs) const {
    return global == rhs.global && local == rhs.local;
  }

  bool operator!=(const LabelName &rhs) const { return !(*this == rhs); }
};

int roundUp(int number, int multiple) {
  assert(multiple > 0);
  int remainder = number % multiple;
  if (remainder == 0)
    return number;
  return number + multiple - remainder;
}

} // namespace
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
// Here, we describe the implementation of the assembler. For details about
// the resulted [ObjectFile], see object_file.h.
//     An [Assembler] takes a vector of string where each string represents a
// line in the source file. We assume each line contains either a directive, a
// label, or and instruction.
//     In brief, the assembling process consists of two passes. In the first
// pass, some preliminary works such as allocating spaces and recoding labels
// are done. In the second pass, instructions are parsed.
// The first pass: (cf. void prepare();)
// * [storage] is resized properly and .align are handled. (Other directives
//   are ignored.)
// * Labels are handled. For a local label ".L1:", it will be renamed to
//   "#Global#L1:" where "Global" is the corresponding global label. Note that
//   local labels appearing in instructions are also renamed. After that, the
//   [labelName2Position] and [globalSymbols] are constructed where
//   [labelName2Position] records the position of each label in [storage] and
//   [globalSymbols] records the global symbols. By default, all non-local
//   symbols are global.
// * Also, we initialize all bytes in [storage] to 0xff instead of 0, which
//   might be helpful in recognizing accessing to uninitialized data.
// The second pass: (cf. void parseCurrentLine(LineIter iter);)
// TODO
class Assembler {
  using LineIter = std::vector<std::string>::const_iterator;

public:
  explicit Assembler(std::vector<std::string> src)
      : src(std::move(src)), curSection(Section::ERROR) {}

  ObjectFile assemble() {
    prepare();
    for (auto iter = src.begin(); iter != src.end(); ++iter) {
      parseCurrentLine(iter);
    }
    storage.resize(roundUp(storage.size(), 16));

    std::unordered_map<std::string, std::size_t> globalSymbol2Pos;
    for (auto &sym : globalSymbols)
      globalSymbol2Pos.emplace(sym, labelName2Position.at({sym, ""}));
    return {std::move(storage), std::move(instsAndPos), globalSymbol2Pos,
            std::move(containsExternalLabel)};
  }

private:
  // Compute labelName2Position
  // For each local label, including the one appeared in instructions, rename it
  //   as #GlobalLabel#LocalLabel:
  // Add global labels
  // Allocate storage
  void prepare() {
    LabelName lastLabel;

    for (auto &line : src) {
      auto tokens = split(line, " ,\t");

      if (isDirective(line)) {
        if (tokens.at(0) != ".align")
          continue;
        auto alignment = std::stoi(tokens.at(1));
        storage.resize(roundUp(storage.size(), alignment));
        continue;
      }
      if (tokens.size() == 1 && tokens[0].back() == ':') { // is label
        // TODO: handle non-text labels?
        auto label = tokens.at(0);
        label.pop_back();           // remove ':'
        if (label.front() == '.') { // local label
          assert(!lastLabel.global.empty());
          label = label.substr(1);
          lastLabel.local = label;
          line = "#" + lastLabel.global + "#" + lastLabel.local + ":";
        } else { // global label
          lastLabel.global = label;
          lastLabel.local = "";
          globalSymbols.emplace(label);
        }
        labelName2Position.emplace(lastLabel, storage.size());
        continue;
      }
      // handle instructions
      std::string opName = tokens.at(0);
      if (tokens.back().front() == '.') { // rename the local label
        assert(!lastLabel.global.empty());
        tokens.back() = "#" + lastLabel.global + "#" + tokens.back().substr(1);
        std::string newLine = tokens.at(0) + " ";
        for (std::size_t i = 1; i < tokens.size(); ++i)
          newLine += tokens[i] + ",";
        newLine.pop_back();
        line = newLine;
      }
      if (auto insts = decomposePseudoInst(tokens); !insts.empty()) {
        // pseudo instructions may be decomposed into several instructions
        storage.resize(storage.size() + 4 * insts.size());
        continue;
      }
      storage.resize(storage.size() + 4);
    }
    std::fill(storage.begin(), storage.end(), std::byte(-1));
  }

  void parseCurrentLine(LineIter iter) {
    auto line = *iter;
    auto tokens = split(line, " ,\t");

    if (isDirective(line)) {
      handleDerivative(tokens);
      return;
    }
    if (tokens.size() == 1 && tokens[0].back() == ':') { // is a label
      return;
    }
    handleInstruction(tokens, iter - src.begin());
  }

private:
  void handleDerivative(const std::vector<std::string> &tokens) {
    assert(!tokens.empty());

    if (tokens[0] == ".global") {
      globalSymbols.insert(tokens.at(1));
      return;
    }
    if (tokens[0] == ".text") {
      curSection = Section::TEXT;
      return;
    }
    if (tokens[0] == ".align") {
      curStoragePos = roundUp(curStoragePos, std::stoi(tokens.at(1)));
    }

    std::cerr << "Ignoring derivative: ";
    for (auto &tok : tokens)
      std::cerr << tok << ' ';
    std::cerr << std::endl;
  }

  // If [str] is a number, then return it.
  // If [str] is a non-external label, then compute the offset if possible
  std::optional<int> getOffset(std::string str) const {
    if (std::isdigit(str.front())) {
      return std::stoi(str, nullptr, 0);
    }
    // is a label
    if (str.front() != '#') { // global
      auto pos = get(labelName2Position, LabelName(str, ""));
      if (!pos)
        return std::nullopt;
      return (std::int64_t)pos.value() - (std::int64_t)curStoragePos;
    }
    // local
    auto labels = split(str, "#");
    assert(labels.size() == 2);
    auto pos = labelName2Position.at(LabelName(labels[0], labels[1]));
    return (std::int64_t)pos - (std::int64_t)curStoragePos;
  }

  void handleInstruction(const std::vector<std::string> &tokens,
                         std::size_t linePos /* to be removed */) {
    assert(!tokens.empty());

    // handle pseudo instructions
    if (tokens[0] == "call") {
      handleCall(tokens);
      return;
    }
    if (auto insts = decomposePseudoInst(tokens); !insts.empty()) {
      assert(insts.size() == 1); // TODO: tail, li
      handleNonPseudoInst(insts[0]);
      return;
    }
    handleNonPseudoInst(tokens);
  }

  void handleCall(const std::vector<std::string> &tokens) {
    using namespace std::string_literals;
    assert(tokens.size() == 2);
    assert(tokens[0] == "call");
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
  }

  void handleNonPseudoInst(const std::string &inst) {
    handleNonPseudoInst(split(inst, " \t,"));
  }

  void handleNonPseudoInst(const std::vector<std::string> &tokens) {
    auto inst = parseInst(tokens);
    *(std::uint32_t *)(storage.data() + curStoragePos) = instsAndPos.size();
    instsAndPos.emplace_back(inst, curStoragePos);
    curStoragePos += 4;
  }

  // Note: call and tail are handled in a different way
  std::vector<std::string>
  decomposePseudoInst(const std::vector<std::string> &tokens) const {
    using namespace std::string_literals;
    assert(!tokens.empty());
    if (tokens[0] == "nop") {
      return {"addi x0, x0, 0"s};
    }
    if (tokens[0] == "li") {
      auto imm = std::stoi(tokens.at(2), nullptr, 0);
      assert(unsigned(imm) <= 0xfff); // TODO
      return {"ori "s + tokens.at(1) + ", x0, "s + tokens.at(2)};
    }
    if (tokens[0] == "mv") {
      return {"addi "s + tokens.at(1) + "," + tokens.at(2) + ",0"};
    }

    static const std::unordered_map<std::string, std::string> branchPair = {
        {"bgt", "blt"}, {"ble", "bge"}, {"bgtu", "bltu"}, {"bleu", "bgeu"}};
    if (auto op = get(branchPair, tokens[0])) {
      return {op.value() + " " + tokens.at(2) + "," + tokens.at(1) + "," +
              tokens.at(3)};
    }

    if (tokens[0] == "j") {
      return {"jal x0, "s + tokens.at(1)};
    }
    if (tokens[0] == "jal" && tokens.size() == 2) {
      return {"jal x1, "s + tokens[1]};
    }
    if (tokens[0] == "jr") {
      return {"jalr x0, 0(" + tokens.at(1) + ")"};
    }
    if (tokens[0] == "jalr" && tokens.size() == 2) {
      return {"jalr x1, 0(" + tokens.at(1) + ")"};
    }
    if (tokens[0] == "ret") {
      return {"jalr x0, 0(x1)"s};
    }
    if (tokens[0] == "call" || tokens[0] == "tail") {
      return {"", ""}; // placeholders
    }

    return {};
  }

private: // current state and intermediate results
  std::vector<std::string> src;
  enum class Section { ERROR = 0, TEXT, DATA, RODATA, BSS } curSection;
  std::size_t curStoragePos = 0;

private: // final results
  std::vector<std::byte> storage;
  std::vector<std::pair<std::shared_ptr<inst::Instruction>, std::size_t>>
      instsAndPos;

  std::unordered_map<LabelName, std::size_t> labelName2Position;
  std::unordered_set<std::string> globalSymbols;
  // When we encounter an instruction which contains an external label, then
  // we add the instruction ID and the label name into this.
  std::unordered_map<inst::Instruction::Id, std::string> containsExternalLabel;
};

} // namespace

} // namespace ravel

namespace ravel {

ObjectFile assemble(const std::string &src) {
  auto lines = preprocess(src);
  Assembler assembler(lines);
  return assembler.assemble();
}

} // namespace ravel