#include "assembler.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "container_utils.h"
#include "instructions.h"
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
class Assembler {
  using LineIter = std::vector<std::string>::const_iterator;

public:
  explicit Assembler(std::vector<std::string> src)
      : src(std::move(src)), curSection(Section::ERROR) {}

  void assemble() {
    prepare();
    for (auto iter = src.begin(); iter != src.end(); ++iter) {
      parseCurrentLine(iter);
    }
  }

private:
  // Compute labelName2Position
  // For each local label, including the one appeared in instructions, rename it
  //   as #GlobalLabel#LocalLabel:
  // Allocate storage
  void prepare() {
    LabelName lastLabel;

    for (auto &line : src) {
      auto tokens = split(line, " ,\t");

      if (isDirective(line)) {
        if (tokens.at(0) !=  ".align")
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
        }
        labelName2Position.emplace(lastLabel, storage.size());
        continue;
      }
      // handle instructions
      std::string opName = tokens.at(0);
      if (tokens.back().front() == '.')  {  // rename the local label
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
  // If [str] is a non-external label, then compute the offset.
  std::optional<int> getOffset(std::string str) const {
    if (std::isdigit(str.front())) {
      return std::stoi(str, nullptr, 0);
    }
    // is a label
    if (str.front() != '#') { // global
      return get(labelName2Position, LabelName(str, ""));
    }
    // local
    auto labels = split(str, "#");
    assert(labels.size() == 2);
    return labelName2Position.at(LabelName(labels[0], labels[1]));
  }

  void handleInstruction(const std::vector<std::string> &tokens,
                         std::size_t linePos) {
    // TODO: pseudo instructions
    assert(!tokens.empty());

    // handle pseudo insts
    if (auto insts = decomposePseudoInst(tokens); !insts.empty()) {
      assert(insts.size() == 1); // TODO: call, tail, li
      handleNonPseudoInst(split(insts[0], " \t,"));
      return;
    }
    handleNonPseudoInst(tokens);
  }

  void handleNonPseudoInst(const std::vector<std::string> &tokens) {
    auto inst = parseInst(tokens);
    *(std::uint32_t *)(storage.data() + curStoragePos) = text.size();
    text.emplace_back(inst);
    curStoragePos += 4;
  }

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
        {"bgt", "blt"}, {"ble", "bge"}, {"bgtu", "bltu"}, {"bleu", "bgeu"}
    };
    if (auto op = get(branchPair, tokens[0])) {
      return {op.value() + " " + tokens.at(2) + "," + tokens.at(1) + "," + tokens.at(3)};
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
      assert(false);  // TODO
      std::string auipc, jalr;
      if (std::optional<int> offsetOpt = getOffset(tokens.at(1))) {
        auto offset = offsetOpt.value();
        auipc = "auipc x6, " + std::to_string(offset >> 12);
        jalr = "jalr "s + (tokens[0] == "call" ? "x1"s : "x0"s) + ", x6, " +
               std::to_string(offset & 0xfff);
      } else {
        auipc = "auipc x6, " + tokens.at(1);
        jalr = "jalr "s + (tokens[0] == "call" ? "x1"s : "x0"s) + ", x6, " +
               tokens.at(1);
      }
      return {auipc, jalr};
    }

    return {};
  }

private: // current state and intermediate results
  std::vector<std::string> src;
  enum class Section { ERROR = 0, TEXT, DATA, RODATA, BSS } curSection;
  std::size_t curStoragePos = 0;

private: // final results
  std::vector<std::byte> storage;
  std::vector<std::shared_ptr<inst::Instruction>> text;

  std::unordered_map<LabelName, std::size_t> labelName2Position;
  std::unordered_set<std::string> globalSymbols;
  // When we encounter an instruction which contains an external label, then
  // we add the instruction ID and the label name into this.
  std::unordered_map<inst::Instruction::Id, LabelName> containsExternalLabel;
};

} // namespace

} // namespace ravel

namespace ravel {

void assemble(const std::string &src) {
  auto lines = preprocess(src);
  Assembler assembler(lines);
  assembler.assemble();
}

} // namespace ravel