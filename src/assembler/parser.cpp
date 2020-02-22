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

  assert(name == "fp");
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

std::string translatePseudoInst(const std::string &line) {
  using namespace std::string_literals;
  auto tokens = tokenize(line);
  assert(!tokens.empty());
  if (tokens[0] == "nop") {
    return "addi x0, x0, 0"s;
  }
  if (tokens[0] == "li") {
    assert(false);
  }
  if (tokens[0] == "mv") {
    return "addi "s + tokens.at(1) + "," + tokens.at(2) + ",0";
  }
  if (tokens[0] == "not") {
    return "xori "s + tokens.at(1) + ", " + tokens.at(2) + ", -1";
  }
  if (tokens[0] == "neg") {
    return "sub "s + tokens.at(1) + ", x0, " + tokens.at(2);
  }

  static const std::unordered_map<std::string, std::string> branchPair = {
      {"bgt", "blt"}, {"ble", "bge"}, {"bgtu", "bltu"}, {"bleu", "bgeu"}};
  if (auto op = get(branchPair, tokens[0])) {
    return op.value() + " " + tokens.at(2) + "," + tokens.at(1) + "," +
           tokens.at(3);
  }

  if (tokens[0] == "j") {
    return "jal x0, "s + tokens.at(1);
  }
  if (tokens[0] == "jal" && tokens.size() == 2) {
    return "jal x1, "s + tokens[1];
  }
  if (tokens[0] == "jr") {
    return "jalr x0, 0(" + tokens.at(1) + ")";
  }
  if (tokens[0] == "jalr" && tokens.size() == 2) {
    return "jalr x1, 0(" + tokens.at(1) + ")";
  }
  if (tokens[0] == "ret") {
    return "jalr x0, 0(x1)"s;
  }
  if (tokens[0] == "call" || tokens[0] == "tail") {
    assert(false && "use handleCall to handle call/tail ");
  }
  return line;
}

std::string parseSectionDerivative(const std::string &line) {
  auto tokens = split(line, " \t,.");
  assert(tokens.at(0) == "section");
  auto name = tokens.at(1);
  return "." + name;
}

std::optional<std::uint32_t> parseImm(const std::string &str) {
  if (str.front() == '%')
    return std::nullopt;
  return std::stoul(str, nullptr, 0);
}

} // namespace ravel