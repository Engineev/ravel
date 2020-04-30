#include "parser.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>
#include <unordered_map>

#include "container_utils.h"
#include "error.h"

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

  // myList.erase(
  //    std::remove_if(myList.begin(), myList.end(), IsMarkedToDelete),
  //    myList.end());
  words.erase(std::remove(words.begin(), words.end(), ""), words.end());

  return words;
}

std::vector<std::string> tokenize(const std::string &line) {
  auto tokens = split(line, " ,\t");
  if (tokens.at(0) == ".string") {
    tokens = {".string", strip(line.substr(7))};
  }
  return tokens;
}

bool isDirective(const std::string &str) {
  auto words = split(str);
  return words.at(0).at(0) == '.' && words.at(0).back() != ':';
}

bool isLabel(const std::string &str) { return str.back() == ':'; }

std::optional<std::string> getSectionName(const std::string &line) {
  if (line == ".text" || line == ".data" || line == ".rodata" || line == ".bss")
    return line;
  auto words = split(line, " \t");
  if (words.at(0) != ".section")
    return std::nullopt;
  return words.at(1);
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
      {inst::Instruction::MUL, "MUL"s},
      {inst::Instruction::MULH, "MULH"s},
      {inst::Instruction::MULHSU, "MULHSU"s},
      {inst::Instruction::MULHU, "MULHU"s},
      {inst::Instruction::DIV, "DIV"s},
      {inst::Instruction::DIVU, "DIVU"s},
      {inst::Instruction::REM, "REM"s},
      {inst::Instruction::REMU, "REMU"s},
  };
  auto name = mp.at(op);
  for (auto &c : name)
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
      {"MUL"s, inst::Instruction::MUL},
      {"MULH"s, inst::Instruction::MULH},
      {"MULHSU"s, inst::Instruction::MULHSU},
      {"MULHU"s, inst::Instruction::MULHU},
      {"DIV"s, inst::Instruction::DIV},
      {"DIVU"s, inst::Instruction::DIVU},
      {"REM"s, inst::Instruction::REM},
      {"REMU"s, inst::Instruction::REMU},
  };

  for (auto &c : name) {
    c = toupper((unsigned char)c);
  }

  return mp.at(name);
}

std::size_t regName2regNumber(const std::string &name) {
  if (name.at(0) == 'x') {
    if (name.size() == 1 || !std::isdigit(name.at(1))) {
      throw Exception("Unknown register: " + name);
    }
    return (std::size_t)std::stoi(name.substr(1));
  }

  static std::vector<std::string> names = {
      "zero", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
      "a1",   "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
      "s6",   "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};
  assert(names.size() == 32);
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (names[i] == name)
      return i;
  }

  if (name != "fp")
    throw Exception("Unknown register: " + name);
  return 8;
}

std::pair<std::size_t, int> parseBaseOffset(const std::string &str) {
  std::stringstream ss(str);
  int offset;
  ss >> offset;
  auto ch = (char)ss.get();
  assert(ch == '(');
  std::string regName;
  ss >> regName;
  assert(regName.back() == ')');
  regName.pop_back();
  auto reg = regName2regNumber(regName);
  return {reg, offset};
}

std::string toString(const std::shared_ptr<inst::Instruction> &inst) {
  auto getName = [](std::size_t num) { return regNumber2regName(num); };
  auto opName = opType2Name(inst->getOp());
  if (auto p = std::dynamic_pointer_cast<inst::ImmConstruction>(inst)) {
    return opName + " " + getName(p->getDest()) + ", " +
           std::to_string(p->getImm());
  }
  if (auto p = std::dynamic_pointer_cast<inst::ArithRegReg>(inst)) {
    return opName + " " + getName(p->getDest()) + ", " + getName(p->getSrc1()) +
           ", " + getName(p->getSrc2());
  }
  if (auto p = std::dynamic_pointer_cast<inst::ArithRegImm>(inst)) {
    return opName + " " + getName(p->getDest()) + ", " + getName(p->getSrc()) +
           ", " + std::to_string(p->getImm());
  }
  if (auto p = std::dynamic_pointer_cast<inst::MemAccess>(inst)) {
    return opName + " " + getName(p->getReg()) + ", " +
           std::to_string(p->getOffset()) + "(" + getName(p->getBase()) + ")";
  }
  if (auto p = std::dynamic_pointer_cast<inst::JumpLink>(inst)) {
    return opName + " " + getName(p->getDest()) + ", " +
           std::to_string(p->getOffset());
  }
  if (auto p = std::dynamic_pointer_cast<inst::JumpLinkReg>(inst)) {
    return opName + " " + getName(p->getDest()) + ", " +
           std::to_string(p->getOffset()) + "(" + getName(p->getBase()) + ")";
  }
  if (auto p = std::dynamic_pointer_cast<inst::Branch>(inst)) {
    return opName + " " + getName(p->getSrc1()) + ", " + getName(p->getSrc2()) +
           ", " + std::to_string(p->getOffset());
  }
  if (auto p = std::dynamic_pointer_cast<inst::MArith>(inst)) {
    return opName + " " + getName(p->getDest()) + ", " + getName(p->getSrc1()) +
           ", " + getName(p->getSrc2());
  }

  assert(false);
}

std::string regNumber2regName(const std::size_t &num) {
  static std::vector<std::string> names = {
      "zero", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
      "a1",   "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
      "s6",   "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};
  return names.at(num);
}

std::string parseSectionDerivative(const std::string &line) {
  auto tokens = split(line, " \t,.");
  assert(tokens.at(0) == "section");
  auto name = tokens.at(1);
  if (name.front() == 's') {
    assert(name == "sdata" || name == "srodata" || name == "sbss");
    name = name.substr(1);
  }
  return "." + name;
}

std::uint32_t parseImm(const std::string &str) {
  return std::stoul(str, nullptr, 0);
}

std::string handleEscapeCharacters(const std::string &str) {
  static std::unordered_map<char, char> escape = {
      {'\'', '\''}, {'\"', '\"'}, {'\\', '\\'}, {'n', '\n'}, {'r', '\r'},
      {'t', '\t'},  {'b', '\b'},  {'f', '\f'},  {'v', '\v'},
  };

  std::string res;
  for (std::size_t i = 0; i < str.length(); ++i) {
    if (str[i] != '\\') {
      res.push_back(str[i]);
      continue;
    }
    ++i;
    assert(i < str.length());

    if (auto chOpt = get(escape, str[i])) {
      res.push_back(chOpt.value());
      continue;
    }
    if (std::isdigit(str[i])) {
      int n = 0;
      for (int k = 0; k < 3 && i < str.length(); ++k, ++i) {
        char ch = str[i];
        if (!isdigit(ch) || ch - '0' >= 8)
          break;
        n *= 8;
        n += ch - '0';
      }
      --i;
      assert(0 <= n && n <= 255);
      res.push_back((char)n);
      continue;
    }
    assert(false && "Unsupported escape characters");
  }
  return res;
}

} // namespace ravel
