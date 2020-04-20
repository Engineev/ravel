#include "preprocessor.h"

#include <cassert>
#include <random>
#include <regex>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include "container_utils.h"
#include "error.h"
#include "parser.h"

namespace ravel {
namespace {

std::vector<std::string> splitIntoLines(const std::string &src) {
  std::vector<std::string> lines;

  std::stringstream ss(src);
  for (std::string line; std::getline(ss, line, '\n');)
    lines.push_back(line);

  return lines;
}

std::string removeComments(const std::string &line) {
  // be careful with the case: .string "#"
  // remove comments
  std::size_t pos = std::string::npos;
  bool inString = false;
  for (std::size_t i = 0; i < line.size(); ++i) {
    auto ch = line[i];
    if (!inString && ch == '#') {
      pos = i;
      break;
    }
    if (ch == '"') {
      inString = !inString;
      continue;
    }
    if (inString && ch == '\\') {
      ++i;
      continue;
    }
  }
  if (pos == std::string::npos)
    return line;
  return line.substr(0, pos);
}

std::vector<std::string> extractLabels(std::vector<std::string> lines) {
  auto bak = lines;
  lines.clear();
  std::regex re("[.a-zA-Z0-9_]*:", std::regex_constants::ECMAScript);
  for (const auto &line : bak) {
    std::smatch matchRes;
    std::regex_search(line, matchRes, re);
    if (matchRes.empty()) {
      lines.emplace_back(line);
      continue;
    }
    assert(matchRes.size() == 1); // at most one label per line
    if (matchRes[0].first != line.begin()) {
      lines.emplace_back(line);
      continue;
    }
    lines.emplace_back(matchRes[0].first, matchRes[0].second);
    assert(matchRes[0].second >= matchRes[0].first);
    if (std::size_t(matchRes[0].second - matchRes[0].first) == line.length())
      continue;
    lines.emplace_back(matchRes[0].second, line.cend());
    lines.back() = strip(lines.back());
    if (lines.back().empty())
      lines.pop_back();
  }
  return lines;
}

std::string translatePseudoInstruction(const std::string &line) {
  using namespace std::string_literals;
  auto tokens = tokenize(line);
  auto opname = tokens.at(0);
  if (opname == "li" || opname == "call" || opname == "tail") {
    assert(false);
  }
  if (opname == "nop") {
    return "addi x0, x0, 0"s;
  }
  if (opname == "mv") {
    return "addi "s + tokens.at(1) + "," + tokens.at(2) + ",0";
  }
  if (opname == "not") {
    return "xori "s + tokens.at(1) + ", " + tokens.at(2) + ", -1";
  }
  if (opname == "neg") {
    return "sub "s + tokens.at(1) + ", x0, " + tokens.at(2);
  }
  if (opname == "seqz") {
    return "sltiu " + tokens.at(1) + ", " + tokens.at(2) + ", 1";
  }
  if (opname == "snez") {
    return "sltu " + tokens.at(1) + ", x0, " + tokens.at(2);
  }
  if (opname == "sltz") {
    return "slt " + tokens.at(1) + ", " + tokens.at(2) + ", x0";
  }
  if (opname == "sgtz") {
    return "slt " + tokens.at(1) + ", x0, " + tokens.at(2);
  }

  if (opname == "sgt") {
    return "slt " + tokens.at(1) + ", " + tokens.at(3) + ", " + tokens.at(2);
  }

  static const std::unordered_map<std::string, std::string> branchPair = {
      {"bgt", "blt"}, {"ble", "bge"}, {"bgtu", "bltu"}, {"bleu", "bgeu"}};
  if (auto op = get(branchPair, opname)) {
    return op.value() + " " + tokens.at(2) + "," + tokens.at(1) + "," +
           tokens.at(3);
  }
  if (opname.front() == 'b' && opname.back() == 'z') {
    opname.pop_back();
    bool reverse = isIn(branchPair, opname);
    if (reverse)
      opname = branchPair.at(opname);
    auto rs1 = tokens.at(1);
    auto rs2 = "x0"s;
    if (reverse)
      std::swap(rs1, rs2);
    return opname + " " + rs1 + ", " + rs2 + ", " + tokens.at(2);
  }

  if (opname == "j") {
    return "jal x0, "s + tokens.at(1);
  }
  if (opname == "jal" && tokens.size() == 2) {
    return "jal x1, "s + tokens[1];
  }
  if (opname == "jr") {
    return "jalr x0, 0(" + tokens.at(1) + ")";
  }
  if (opname == "jalr" && tokens.size() == 2) {
    return "jalr x1, 0(" + tokens.at(1) + ")";
  }
  if (opname == "ret") {
    return "jalr x0, 0(x1)"s;
  }

  return line;
}

std::vector<std::string>
translatePseudoInstructions(std::vector<std::string> lines) {
  std::mt19937_64 eng(std::random_device{}());
  std::uniform_int_distribution<char> randomChar('a', 'z');
  std::string prefix;
  for (int i = 0; i < 8; ++i)
    prefix.push_back(randomChar(eng));
  prefix += "_pseudo_inst_label_";
  std::size_t newLabelCnt = 0;
  auto newLabel = [&prefix, &newLabelCnt] {
    return prefix + std::to_string(newLabelCnt++);
  };

  auto bak = lines;
  lines.clear();
  for (auto &lineBak : bak) {
    if (isLabel(lineBak) || isDirective(lineBak)) {
      lines.emplace_back(lineBak);
      continue;
    }
    auto tokens = tokenize(lineBak);
    auto op = tokens.front();

    // la, l{b|h|w}, s{b|h|w|d}
    static const std::unordered_set<std::string> Mem = {"lb", "lh", "lw",
                                                        "sb", "sh", "sw"};
    // Note: Non-pseudo loads are for the form: lw rd, offset(reg)
    if (op == "la" || (isIn(Mem, op) && tokens.at(2).back() != ')')) {
      auto label = newLabel();
      auto rd = tokens.at(1);
      auto symbol = tokens.at(2);
      lines.emplace_back(label + ":");
      if (op == "la") {
        lines.emplace_back("auipc " + rd + ", %pcrel_hi(" + symbol + ")");
        lines.emplace_back("addi " + rd + ", " + rd + ", %pcrel_lo(" + label +
                           ")");
        continue;
      }
      if (op.front() == 'l') {
        lines.emplace_back("auipc " + rd + ", %pcrel_hi(" + symbol + ")");
        lines.emplace_back(op + " " + rd + ", %pcrel_lo(" + label + ")(" + rd +
                           ")");
        continue;
      }
      if (op.front() == 's') {
        auto rt = tokens.at(3);
        lines.emplace_back("auipc " + rt + ", %pcrel_hi(" + symbol + ")");
        lines.emplace_back(op + " " + rd + ", %pcrel_lo(" + label + ")(" + rt +
                           ")");
        continue;
      }
      assert(false);
    }

    // li
    if (op == "li") {
      auto rd = tokens.at(1);
      auto imm = std::stoi(tokens.at(2), nullptr, 0);
      auto uImm = std::uint32_t(imm) >> 12u;
      auto lImm = std::uint32_t(imm) & 0xfffu;
      if (uImm != 0) { // large imm
        lines.emplace_back("lui " + rd + ", " + std::to_string(uImm));
        lines.emplace_back("ori " + rd + ", " + rd + ", " +
                           std::to_string(lImm));
      } else {
        lines.emplace_back("addi " + rd + ", zero, " + std::to_string(lImm));
      }
      continue;
    }

    // call, tail
    if (op == "call" || op == "tail") {
      using namespace std::string_literals;
      auto funcName = tokens.at(1);
      auto label = newLabel();
      lines.emplace_back(label + ":");
      lines.emplace_back("auipc x6, %pcrel_hi(" + funcName + ")");
      lines.emplace_back("jalr "s + (op == "call" ? "x1"s : "x0"s) +
                         ", %pcrel_lo(" + label + ")(x6)");
      continue;
    }

    lines.emplace_back(translatePseudoInstruction(lineBak));
  }

  return lines;
}

} // namespace

std::vector<std::string> preprocess(const std::string &src) {
  auto lines = splitIntoLines(src);
  for (auto &line : lines) {
    line = removeComments(line);
    line = strip(line);
  }
  lines = extractLabels(std::move(lines));
  // remove empty lines
  lines.erase(std::remove_if(lines.begin(), lines.end(),
                             [](auto &l) { return l.empty(); }),
              lines.end());
  return translatePseudoInstructions(std::move(lines));
}

} // namespace ravel