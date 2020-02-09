#include "interpreter.h"

#include <cassert>

#include "parser.h"

namespace ravel {
namespace {

template <class T>
std::shared_ptr<T> dpc(const std::shared_ptr<inst::Instruction> &inst) {
  return std::dynamic_pointer_cast<T>(inst);
}

template <class T>
const T &spc(const std::shared_ptr<inst::Instruction> &inst) {
  return static_cast<const T &>(*inst);
}

} // namespace

void Interpreter::simulate(const std::shared_ptr<inst::Instruction> &inst) {
  // Do NOT use dynamic cast, it is too time-consuming
  using Op = inst::Instruction::OpType;

  auto resetZero =
      std::shared_ptr<void>(nullptr, [this](void *) { regs[0] = 0; });
  auto pcAdd4 = std::shared_ptr<void>(nullptr, [this](void *) { pc += 4; });

  // ImmConstruction
  if (inst->getOp() == inst::Instruction::LUI) {
    auto &p = spc<inst::ImmConstruction>(inst);
    regs.at(p.getDest()) = (std::uint32_t)p.getImm() & 0xfffff000;
    return;
  }
  if (inst->getOp() == inst::ImmConstruction::AUIPC) {
    auto &p = spc<inst::ImmConstruction>(inst);
    auto offset = (std::uint32_t)p.getImm() & 0xfffff000;
    pc += offset;
    return;
  }

  // ArithRegReg & ArithRegImm
  if (bool isArithRegReg = inst::Instruction::ADD <= inst->getOp() &&
                           inst->getOp() <= inst::Instruction::AND,
      isArithRegImm = inst::Instruction::ADDI <= inst->getOp() &&
                      inst->getOp() <= inst::Instruction::SRAI;
      isArithRegReg || isArithRegImm) {
    std::uint32_t rs1, rs2;
    std::size_t destNumber;
    if (isArithRegReg) {
      auto &p = spc<inst::ArithRegReg>(inst);
      destNumber = p.getDest();
      rs1 = regs.at(p.getSrc1());
      rs2 = regs.at(p.getSrc2());
    } else {
      auto &p = spc<inst::ArithRegImm>(inst);
      destNumber = p.getDest();
      rs1 = regs.at(p.getSrc());
      rs2 = p.getImm();
    }
    auto &dest = regs.at(destNumber);

    switch (inst->getOp()) {
    case Op::ADD:
    case Op::ADDI:
      dest = rs1 + rs2;
      return;
    case Op::SUB:
      dest = (std::int32_t)rs1 - (std::int32_t)rs2;
      return;
    case Op::SLL:
    case Op::SLLI:
      dest = rs1 << rs2;
      return;
    case Op::SLT:
    case Op::SLTI:
      dest = (std::int32_t)rs1 < (std::int32_t)rs2;
    case Op::SLTU:
    case Op::SLTIU:
      dest = rs1 < rs2;
      return;
    case Op::XOR:
    case Op::XORI:
      dest = rs1 ^ rs2;
      return;
    case Op::SRL:
    case Op::SRLI:
      dest = rs1 >> rs2;
      return;
    case Op::SRA:
    case Op::SRAI:
      dest = (std::int32_t)rs1 >> rs2;
      return;
    case Op::OR:
    case Op::ORI:
      dest = rs1 | rs2;
      return;
    case Op::AND:
    case Op::ANDI:
      dest = rs1 & rs2;
      return;
    default:
      assert(false);
    }
  }

  // MemAccess
  if (Op::LB <= inst->getOp() && inst->getOp() <= Op::SW) {
    auto &p = spc<inst::MemAccess>(inst);
    std::byte *addr = storage.data() + regs.at(p.getBase()) + p.getOffset();
    assert(storage.data() <= addr && addr < storage.data() + storage.size());
    switch (inst->getOp()) {
    case Op::SB:
      *(std::uint8_t *)addr = regs.at(p.getReg());
      return;
    case Op::SH:
      *(std::uint16_t *)addr = regs.at(p.getReg());
      return;
    case Op::SW:
      *(std::uint32_t *)addr = regs.at(p.getReg());
      return;
    case Op::LB:
      regs.at(p.getReg()) = *(std::int8_t *)addr;
      return;
    case Op::LH:
      regs.at(p.getReg()) = *(std::int16_t *)addr;
      return;
    case Op::LW:
      regs.at(p.getReg()) = *(std::int32_t *)addr;
      return;
    case Op::LBU:
      (std::uint32_t &)regs.at(p.getReg()) = *(std::uint8_t *)addr;
      return;
    case Op::LHU:
      (std::uint32_t &)regs.at(p.getReg()) = *(std::uint16_t *)addr;
      return;
    default:
      assert(false);
    }
  }

  if (inst->getOp() == Op::JAL) {
    auto &p = spc<inst::JumpLink>(inst);
    auto offset = p.getOffset() * 2;
    regs.at(p.getDest()) = pc + 4;
    pc += offset - 4;
    return;
  }

  if (inst->getOp() == Op::JALR) {
    auto &p = spc<inst::JumpLinkReg>(inst);
    regs.at(p.getDest()) = pc + 4;
    auto addr = regs.at(p.getBase()) + p.getOffset();
    addr &= ~1;
    pc = addr - 4;
    return;
  }

  if (Op::BEQ <= inst->getOp() && inst->getOp() <= Op::BGEU) {
    auto &p = spc<inst::Branch>(inst);
    std::int32_t rs1 = regs.at(p.getSrc1());
    std::int32_t rs2 = regs.at(p.getSrc2());
    bool shouldJump;
    switch (inst->getOp()) {
    case Op::BEQ:
      shouldJump = rs1 == rs2;
      break;
    case Op::BNE:
      shouldJump = rs1 != rs2;
      break;
    case Op::BLT:
      shouldJump = rs1 < rs2;
      break;
    case Op::BGE:
      shouldJump = rs1 >= rs2;
      break;
    case Op::BLTU:
      shouldJump = (std::uint32_t)rs1 < (std::uint32_t)rs2;
      break;
    case Op::BGEU:
      shouldJump = (std::uint32_t)rs1 == (std::uint32_t)rs2;
      break;
    default:
      assert(false);
    }
    if (shouldJump)
      pc += p.getOffset() - 4;
    return;
  }

  assert(false);
}

std::uint32_t Interpreter::getReturnCode() const {
  return 0xffff & regs.at(regName2regNumber("a0"));
}

void Interpreter::load() {
  storage.insert(storage.end(), interpretable.getStorage().begin(),
                 interpretable.getStorage().end());
  std::size_t MaxStorageSize = 64 * 1024 * 1024;
  storage.resize(MaxStorageSize);
  pc = interpretable.getStart();
  regs.at(regName2regNumber("sp")) = MaxStorageSize;
}

} // namespace ravel
