#include "interpreter.h"

//#define PRINT_INSTS

#include <algorithm>
#include <cassert>
#include <functional>

#ifdef PRINT_INSTS
#include <iostream>
#endif

#include "assembler/parser.h"
#include "interpreter/libc_sim.h"

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
  // TODO: use switch instead of a chain of if-else
  using Op = inst::Instruction::OpType;
  auto op = inst->getOp();

  // has been moved to Interpreter::interpret()
  // struct Raii {
  //   Raii(std::function<void()> func) : func(std::move(func)) {}
  //   ~Raii() { func(); }
  //   std::function<void()> func;
  // };
  // auto resetZero = Raii([this] { regs[0] = 0; });
  // auto pcAdd4 = Raii([this] { pc += 4; });

  // ImmConstruction
  if (op == inst::Instruction::LUI) {
    ++instCnt.simple;
    auto &p = spc<inst::ImmConstruction>(inst);
    regs.at(p.getDest()) = (std::uint32_t)p.getImm() << 12;
    return;
  }
  if (op == inst::ImmConstruction::AUIPC) {
    ++instCnt.simple;
    auto &p = spc<inst::ImmConstruction>(inst);
    auto offset = (std::uint32_t)p.getImm() << 12;
    regs.at(p.getDest()) = pc + offset;
    return;
  }

  // ArithRegReg & ArithRegImm
  if (bool isArithRegReg =
          inst::Instruction::ADD <= op && op <= inst::Instruction::AND,
      isArithRegImm =
          inst::Instruction::ADDI <= op && op <= inst::Instruction::SRAI;
      isArithRegReg || isArithRegImm) {
    ++instCnt.simple;
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

    switch (op) {
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
  if (Op::LB <= op && op <= Op::SW) {
    ++instCnt.mem;
    auto &p = spc<inst::MemAccess>(inst);
    std::byte *addr = storage.data() + regs.at(p.getBase()) + p.getOffset();
    assert(storage.data() <= addr && addr < storage.data() + storage.size());
    switch (op) {
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

  if (op == Op::JAL) {
    ++instCnt.simple;
    auto &p = spc<inst::JumpLink>(inst);
    auto offset = p.getOffset() * 2;
    regs.at(p.getDest()) = pc + 4;
    pc += offset - 4;
    return;
  }

  if (op == Op::JALR) {
    ++instCnt.simple;
    auto &p = spc<inst::JumpLinkReg>(inst);
    regs.at(p.getDest()) = pc + 4;
    auto addr = regs.at(p.getBase()) + p.getOffset();
    addr &= ~1;
    pc = addr - 4;
    return;
  }

  if (Op::BEQ <= op && op <= Op::BGEU) {
    ++instCnt.br;
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

  if (Op::MUL <= op && op <= Op::REMU) {
    if (Op::MUL <= op && op <= Op::MULHU)
      ++instCnt.mul;
    else
      ++instCnt.div;

    auto &p = spc<inst::MArith>(inst);
    auto destNumber = p.getDest();
    auto &dest = regs.at(destNumber);
    std::uint32_t rs1 = regs.at(p.getSrc1());
    std::uint32_t rs2 = regs.at(p.getSrc2());
    switch (op) {
    case Op::MUL:
      dest = (std::int32_t)rs1 * (std::int32_t)rs2;
      return;
    case Op::MULH:
      dest = (std::uint32_t)(((std::int64_t)rs1 * (std::int64_t)rs2) >> 32);
      return;
    case Op::MULHSU:
      dest = (std::uint32_t)(((std::int64_t)rs1 * (std::uint64_t)rs2) >> 32);
      return;
    case Op::MULHU:
      dest = (std::uint32_t)(((std::uint64_t)rs1 * (std::uint64_t)rs2) >> 32);
      return;
    case Op::DIV:
      dest = (std::int32_t)rs1 / (std::int32_t)rs2;
      return;
    case Op::DIVU:
      dest = (std::uint32_t)rs1 / (std::uint32_t)rs2;
      return;
    case Op::REM:
      dest = (std::int32_t)rs1 % (std::int32_t)rs2;
      return;
    case Op::REMU:
      dest = (std::uint32_t)rs1 % (std::uint32_t)rs2;
      return;
    default:
      assert(false);
    }
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
  heapPtr = storage.size();
  std::size_t MaxStorageSize = 256 * 1024 * 1024;
  assert(storage.size() < 3 * MaxStorageSize / 4);
  storage.resize(MaxStorageSize);
  pc = Interpretable::Start;
  regs.at(regName2regNumber("sp")) = MaxStorageSize;
}

void Interpreter::interpret() {
  load();
  while (pc != Interpretable::End) {
    assert(0 <= pc && pc < storage.size());
    if (Interpretable::LibcFuncStart <= pc && pc < Interpretable::LibcFuncEnd) {
#ifdef PRINT_INSTS
      std::cerr << pc << ": call libc-" << pc << std::endl;
#endif
      simulateLibCFunc(libc::Func(pc));
      pc = regs[1];
      continue;
    }

    auto instIdx = *(std::uint32_t *)(storage.data() + pc);
    auto &inst = interpretable.getInsts().at(instIdx);
#ifdef PRINT_INSTS
    std::cerr << pc << ": " << toString(inst);
    if (!inst->getComment().empty())
      std::cerr << "  # " << inst->getComment();
    std::cerr << std::endl;
#endif
    simulate(inst);
    regs[0] = 0;
    pc += 4;
  }
}

void Interpreter::simulateLibCFunc(libc::Func funcN) {
  if (libc::PUTS <= funcN && funcN <= libc::PUTCHAR)
    ++instCnt.libcIO;
  else if (libc::MALLOC <= funcN && funcN <= libc::MEMSET)
    ++instCnt.libcMem;
  else
    assert(false);

  switch (funcN) {
  case libc::PUTS:
    libc::puts(regs, storage, out);
    return;
  case libc::SCANF:
    libc::scanf(regs, storage, in);
    return;
  case libc::PRINTF:
    libc::printf(regs, storage, out);
    return;
  case libc::PUTCHAR:
    libc::putchar(regs, out);
    return;
  case libc::MALLOC:
    libc::malloc(regs, storage, heapPtr, malloced);
    return;
  case libc::FREE:
    libc::free(regs, malloced);
    return;
  case libc::MEMCPY:
    libc::memcpy(regs, storage);
    return;
  case libc::STRLEN:
    libc::strlen(regs, storage);
    return;
  case libc::STRCPY:
    libc::strcpy(regs, storage);
    return;
  case libc::MEMSET:
    libc::memset(regs, storage);
    return;
  default:
    assert(false);
  }
  assert(false);
}

} // namespace ravel
