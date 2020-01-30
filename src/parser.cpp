#include "parser.h"

#include <algorithm>
#include <regex>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include "container_utils.h"

namespace ravel {

std::string strip(std::string str) {
  auto whitespaces = " \t\n\r\f\v";
  str.erase(str.find_last_not_of(whitespaces) + 1);
  str.erase(0, str.find_first_not_of(whitespaces));
  return str;
}

std::vector<std::string> split(const std::string &s,
                               const std::string &delimiters) {
  std::vector<std::string> words;

  std::size_t next = -1;
  do {
    auto current = next + 1;
    next = s.find_first_of(delimiters, current);
    if (next > current) // no empty word
      words.emplace_back(s.substr(current, next - current));
  } while (next != std::string::npos);

  return words;
}

bool isDirective(const std::string &str) {
  auto words = split(str);
  return words.at(0).at(0) == '.' && words.at(0).back() != ':';
}

std::optional<std::string> getSectionName(const std::string &line) {
  if (line == ".text" || line == ".data" || line == ".rodata" || line == ".bss")
    return line;
  auto words = split(line, " \t");
  if (words.at(0) != ".section")
    return std::nullopt;
  return words.at(1);
}

std::vector<std::string> preprocess(const std::string &src) {
  std::vector<std::string> lines;

  std::stringstream ss(src);
  for (std::string line; std::getline(ss, line, '\n');)
    lines.push_back(line);

  for (auto &line : lines) {
    // trimming
    line = strip(line);

    // remove comments
    auto pos = line.find('#');
    if (pos == std::string::npos)
      continue;
    line.resize(pos);
  }

  // extract labels
  {
    auto bak = lines;
    lines.clear();
    std::regex re("[.a-zA-Z0-9_]*:", std::regex_constants::ECMAScript);
    for (auto &line : bak) {
      std::smatch matchRes;
      std::regex_search(line, matchRes, re);
      if (matchRes.empty()) {
        lines.emplace_back(std::move(line));
        continue;
      }
      assert(matchRes.size() == 1); // at most one label per line
      if (matchRes[0].first != line.begin())
        continue;

      lines.emplace_back(matchRes[0].first, matchRes[0].second);
      lines.emplace_back(matchRes[0].second, line.cend());
      lines.back() = strip(lines.back());
      if (lines.back().empty())
        lines.pop_back();
    }
  }

  // remove empty lines
  lines.erase(std::remove_if(lines.begin(), lines.end(),
                             [](auto &l) { return l.empty(); }),
              lines.end());

  return lines;
}

std::string opType2Name(inst::Instruction::OpType op) {
  using namespace std::string_literals;
  static std::unordered_map<inst::Instruction::OpType, std::string> mp{
      {inst::Instruction::LUI, "LUI"s},
      {inst::Instruction::AUIPC, "AUIPC"s},
      {inst::Instruction::JAL, "JAL"s},
      {inst::Instruction::JALR, "JALR"s},
      {inst::Instruction::BEQ, "BEQ"s},
      {inst::Instruction::BNE, "BNE"s},
      {inst::Instruction::BLT, "BLT"s},
      {inst::Instruction::BGE, "BGE"s},
      {inst::Instruction::BLTU, "BLTU"s},
      {inst::Instruction::BGEU, "BGEU"s},
      {inst::Instruction::LB, "LB"s},
      {inst::Instruction::LH, "LH"s},
      {inst::Instruction::LW, "LW"s},
      {inst::Instruction::LBU, "LBU"s},
      {inst::Instruction::LHU, "LHU"s},
      {inst::Instruction::SB, "SB"s},
      {inst::Instruction::SH, "SH"s},
      {inst::Instruction::SW, "SW"s},
      {inst::Instruction::ADDI, "ADDI"s},
      {inst::Instruction::SLTI, "SLTI"s},
      {inst::Instruction::SLTIU, "SLTIU"s},
      {inst::Instruction::XORI, "XORI"s},
      {inst::Instruction::ORI, "ORI"s},
      {inst::Instruction::ANDI, "ANDI"s},
      {inst::Instruction::SLLI, "SLLI"s},
      {inst::Instruction::SRLI, "SRLI"s},
      {inst::Instruction::SRAI, "SRAI"s},
      {inst::Instruction::ADD, "ADD"s},
      {inst::Instruction::SUB, "SUB"s},
      {inst::Instruction::SLL, "SLL"s},
      {inst::Instruction::SLT, "SLT"s},
      {inst::Instruction::SLTU, "SLTU"s},
      {inst::Instruction::XOR, "XOR"s},
      {inst::Instruction::SRL, "SRL"s},
      {inst::Instruction::SRA, "SRA"s},
      {inst::Instruction::OR, "OR"s},
      {inst::Instruction::AND, "AND"s},
  };
  auto name = mp.at(op);
  for (auto & c : name)
    c = tolower(c);
  return name;
}

inst::Instruction::OpType name2OpType(std::string name) {
  using namespace std::string_literals;
  static std::unordered_map<std::string, inst::Instruction::OpType> mp{
      {"LUI"s, inst::Instruction::LUI},
      {"AUIPC"s, inst::Instruction::AUIPC},
      {"JAL"s, inst::Instruction::JAL},
      {"JALR"s, inst::Instruction::JALR},
      {"BEQ"s, inst::Instruction::BEQ},
      {"BNE"s, inst::Instruction::BNE},
      {"BLT"s, inst::Instruction::BLT},
      {"BGE"s, inst::Instruction::BGE},
      {"BLTU"s, inst::Instruction::BLTU},
      {"BGEU"s, inst::Instruction::BGEU},
      {"LB"s, inst::Instruction::LB},
      {"LH"s, inst::Instruction::LH},
      {"LW"s, inst::Instruction::LW},
      {"LBU"s, inst::Instruction::LBU},
      {"LHU"s, inst::Instruction::LHU},
      {"SB"s, inst::Instruction::SB},
      {"SH"s, inst::Instruction::SH},
      {"SW"s, inst::Instruction::SW},
      {"ADDI"s, inst::Instruction::ADDI},
      {"SLTI"s, inst::Instruction::SLTI},
      {"SLTIU"s, inst::Instruction::SLTIU},
      {"XORI"s, inst::Instruction::XORI},
      {"ORI"s, inst::Instruction::ORI},
      {"ANDI"s, inst::Instruction::ANDI},
      {"SLLI"s, inst::Instruction::SLLI},
      {"SRLI"s, inst::Instruction::SRLI},
      {"SRAI"s, inst::Instruction::SRAI},
      {"ADD"s, inst::Instruction::ADD},
      {"SUB"s, inst::Instruction::SUB},
      {"SLL"s, inst::Instruction::SLL},
      {"SLT"s, inst::Instruction::SLT},
      {"SLTU"s, inst::Instruction::SLTU},
      {"XOR"s, inst::Instruction::XOR},
      {"SRL"s, inst::Instruction::SRL},
      {"SRA"s, inst::Instruction::SRA},
      {"OR"s, inst::Instruction::OR},
      {"AND"s, inst::Instruction::AND},
  };

  for (auto &c : name) {
    c = toupper((unsigned char)c);
  }

  return mp.at(name);
}

std::shared_ptr<inst::Instruction>
parseInst(const std::vector<std::string> &tokens) {
  const std::size_t DummyOffset = 0;
  assert(!tokens.empty());

  auto op = name2OpType(tokens[0]);

  if (op == inst::Instruction::LUI || op == inst::Instruction::AUIPC) {
    auto dest = regName2regNumber(tokens.at(1));
    auto imm = std::stoi(tokens.at(2));
    return std::make_shared<inst::ImmConstruction>(op, dest, imm);
  }

  static std::unordered_set<std::string> arithRegReg = {
      "add", "sub", "sll", "slt", "sltu", "xor", "srl", "sra", "or", "and"
  };
  if (isIn(arithRegReg, tokens[0])) {
    auto dest = regName2regNumber(tokens.at(1));
    auto src1 = regName2regNumber(tokens.at(2));
    auto src2 = regName2regNumber(tokens.at(3));
    return std::make_shared<inst::ArithRegReg>(op, dest, src1, src2);
  }

  static std::unordered_set<std::string> arithRegImmInsts = {
      "addi", "slti", "sltiu", "xori", "ori", "andi", "slli", "srli", "srai"
  };
  if (isIn(arithRegImmInsts, tokens.at(0))) {
    auto dest = regName2regNumber(tokens.at(1));
    auto src = regName2regNumber(tokens.at(2));
    auto imm = std::stoi(tokens.at(3), nullptr, 0);
    return std::make_shared<inst::ArithRegImm>(op, dest, src, imm);
  }

  static std::unordered_set<std::string> memAccessInsts = {
      "lb", "lh", "lw", "lbu", "lhu", "sb", "sh", "sw"
  };
  if (isIn(memAccessInsts, tokens[0])) {
    auto reg = regName2regNumber(tokens.at(1));
    auto [base, offset] = parseBaseOffset(tokens.at(2));
    return std::make_shared<inst::MemAccess>(op, reg, base, offset);
  }

  if (op == inst::Instruction::JAL) {
    auto dest = regName2regNumber(tokens.at(1));
    auto offset = std::stoi(tokens.at(2));
    return std::make_shared<inst::JumpLink>(dest, offset);
  }

  if (op == inst::Instruction::JALR) {
    auto dest = regName2regNumber(tokens.at(1));
    auto [base, offset] = parseBaseOffset(tokens.at(2));
    return std::make_shared<inst::JumpLinkReg>(dest, base, offset);
  }

  static std::unordered_set<std::string> branchInsts = {
      "beq", "bne", "blt", "bge", "bltu", "bgeu"
  };
  if (isIn(branchInsts, tokens[0])) {
    auto src1 = regName2regNumber(tokens.at(1));
    auto src2 = regName2regNumber(tokens.at(2));
    return std::make_shared<inst::Branch>(op, src1, src2, DummyOffset);
  }

  assert(false);
}

std::size_t regName2regNumber(const std::string &name) {
  if (name.at(0) == 'x') {
    return (std::size_t) std::stoi(name.substr(1));
  }

  static std::vector<std::string> names = {
      "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1",
      "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "s2", "s3",
      "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
  };
  assert(names.size() == 32);
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (names[i] == name)
      return i;
  }

  assert(name == "fp");
  return 8;
}

std::pair<std::size_t, int> parseBaseOffset(const std::string &str) {
  std::stringstream ss(str);
  int offset;
  ss >> offset;
  auto ch = (char) ss.get();
  assert(ch == '(');
  std::string regName;
  ss >> regName;
  assert(regName.back() == ')');
  regName.pop_back();
  auto reg = regName2regNumber(regName);
  return {reg, offset};
}


} // namespace ravel
