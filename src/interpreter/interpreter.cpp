#include "ravel/interpreter/interpreter.h"

#include <cassert>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <queue>
#include <stack>

#include "ravel/assembler/parser.h"
#include "ravel/error.h"
#include "ravel/interpreter/libc_sim.h"

namespace ravel {
namespace {

template <class T>
const T &spc(const std::shared_ptr<inst::Instruction> &inst) {
  return static_cast<const T &>(*inst);
}

} // namespace

void Interpreter::simulate(const std::shared_ptr<inst::Instruction> &inst) {
  // Do NOT use dynamic cast. It is too time-consuming
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
      rs1 = regs[p.getSrc1()];
      rs2 = regs[p.getSrc2()];
    } else {
      auto &p = spc<inst::ArithRegImm>(inst);
      destNumber = p.getDest();
      rs1 = regs[p.getSrc()];
      rs2 = p.getImm();
    }
    auto &dest = regs[destNumber];

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
      return;
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
    auto &p = spc<inst::MemAccess>(inst);
    std::size_t vAddr = regs[p.getBase()] + p.getOffset();
    if (keepDebugInfo && (isIn(invalidAddress, vAddr) || vAddr == 0)) {
      // Accessing 0x0 is always invalid since an instruction is stored there.
      // Perform this check since many students use 0x0 as the actual value of
      // null.
      throw InvalidAddress(vAddr);
    }

    std::byte *addr = cache.getMemory().first + vAddr;
    assert(addr < cache.getMemory().second);
    // To avoid memory access error when accessing with byte or half-word, we
    // need to change the address to a proper one.
    // For example, we need to access a memory at one byte below the stack
    // pointer, we cannot fetch a word from the address, since will cause a
    // memory access error. Instead, we need to fetch a word from the address
    // 4 bytes below the stack pointer, and then extract the byte we need.
    std::size_t fetchFrom = vAddr;
    switch (op) {
    case Op::SB:
    case Op::LB:
    case Op::LBU:
      fetchFrom &= ~0b11; // align to 4 bytes
      break;
    case Op::SH:
    case Op::LH:
    case Op::LHU:
      if (vAddr % 4 == 3) {
        fetchFrom -= 2;
      } else {
        fetchFrom &= ~0b11;
      }
      break;
    }
    cache.fetchWord(fetchFrom);
    std::tie(instCnt.cache, instCnt.mem) = cache.getHitMiss();
    switch (op) {
    case Op::SB:
      *(std::uint8_t *)addr = regs[p.getReg()];
      return;
    case Op::SH:
      *(std::uint16_t *)addr = regs[p.getReg()];
      return;
    case Op::SW:
      *(std::uint32_t *)addr = regs[p.getReg()];
      return;
    case Op::LB:
      regs[p.getReg()] = *(std::int8_t *)addr;
      return;
    case Op::LH:
      regs[p.getReg()] = *(std::int16_t *)addr;
      return;
    case Op::LW:
      regs[p.getReg()] = *(std::int32_t *)addr;
      return;
    case Op::LBU:
      (std::uint32_t &)regs[p.getReg()] = *(std::uint8_t *)addr;
      return;
    case Op::LHU:
      (std::uint32_t &)regs[p.getReg()] = *(std::uint16_t *)addr;
      return;
    default:
      assert(false);
    }
  }

  if (Op::BEQ <= op && op <= Op::BGEU) {
    ++instCnt.br;
    auto &p = spc<inst::Branch>(inst);
    std::int32_t rs1 = regs[p.getSrc1()];
    std::int32_t rs2 = regs[p.getSrc2()];
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
      shouldJump = (std::uint32_t)rs1 >= (std::uint32_t)rs2;
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
    auto &dest = regs[destNumber];
    std::uint32_t rs1 = regs[p.getSrc1()];
    std::uint32_t rs2 = regs[p.getSrc2()];
    switch (op) {
    case Op::MUL:
      dest = (std::int32_t)rs1 * (std::int32_t)rs2;
      return;
    case Op::MULH: {
      // Note: uint32 -> int32 -> int64 != uint32 -> int64
      std::int32_t r1 = rs1, r2 = rs2;
      std::int64_t res = (std::int64_t)r1 * (std::int64_t)r2;
      dest = (std::uint32_t)(res >> 32u);
      return;
    }
    case Op::MULHSU: {
      std::int32_t r1 = rs1;
      std::int64_t res = (std::int64_t)r1 * (std::uint64_t)rs2;
      dest = (std::uint32_t)(res >> 32u);
      return;
    }
    case Op::MULHU:
      dest = (std::uint32_t)(((std::uint64_t)rs1 * (std::uint64_t)rs2) >> 32u);
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

  switch (op) {
  case inst::Instruction::LUI: {
    ++instCnt.simple;
    auto &p = spc<inst::ImmConstruction>(inst);
    regs[p.getDest()] = p.getImm() << 12u;
    return;
  }

  case inst::ImmConstruction::AUIPC: {
    ++instCnt.simple;
    auto &p = spc<inst::ImmConstruction>(inst);
    std::int32_t offset = (std::uint32_t)p.getImm() << 12u;
    regs[p.getDest()] = pc + offset;
    return;
  }

  case Op::JAL: {
    ++instCnt.simple;
    auto &p = spc<inst::JumpLink>(inst);
    auto offset = p.getOffset() * 2;
    regs[p.getDest()] = pc + 4;
    pc += offset - 4;
    return;
  }

  case Op::JALR: {
    ++instCnt.simple;
    auto &p = spc<inst::JumpLinkReg>(inst);
    regs[p.getDest()] = pc + 4;
    auto addr = regs[p.getBase()] + p.getOffset();
    addr &= ~1u;
    pc = addr - 4;
    return;
  }

  default:
    assert(false);
  }

  assert(false);
}

std::uint32_t Interpreter::getReturnCode() const {
  return 0xffffu & regs.at(regName2regNumber("a0"));
}

void Interpreter::load() {
  std::copy(interpretable.getStorage().begin(),
            interpretable.getStorage().end(), cache.getMemory().first);
  heapPtr = interpretable.getStorage().size();
  assert(heapPtr < cache.storageSize() / 2);
  pc = Interpretable::Start;
  regs.at(regName2regNumber("sp")) = cache.storageSize();
}

namespace {
struct DebugStackFrame {
  void addInstruction(std::shared_ptr<inst::Instruction> inst) {
    lastFewInstructions.emplace(std::move(inst));
    if (lastFewInstructions.size() > MaxSize)
      lastFewInstructions.pop();
  }

  static constexpr std::size_t MaxSize = 8;
  std::queue<std::shared_ptr<inst::Instruction>> lastFewInstructions;
};
} // namespace

namespace {

void printInstWithComment(const std::shared_ptr<inst::Instruction> &inst) {
  std::cerr << toString(inst);
  if (!inst->getComment().empty())
    std::cerr << "  # " << inst->getComment();
  std::cerr << std::endl;
}

std::optional<std::size_t>
getModifiedReg(const std::shared_ptr<inst::Instruction> &inst) {
  auto op = inst->getOp();
  if (inst::Instruction::LUI <= op && op <= inst::Instruction::AUIPC) {
    return spc<inst::ImmConstruction>(inst).getDest();
  }
  if (op == inst::Instruction::JAL)
    return spc<inst::JumpLink>(inst).getDest();
  if (op == inst::Instruction::JALR)
    return spc<inst::JumpLinkReg>(inst).getDest();
  if (inst::Instruction::LB <= op && op <= inst::Instruction::LHU)
    return spc<inst::MemAccess>(inst).getReg();
  if (inst::Instruction::ADDI <= op && op <= inst::Instruction::SRAI)
    return spc<inst::ArithRegImm>(inst).getDest();
  if (inst::Instruction::ADD <= op && op <= inst::Instruction::AND)
    return spc<inst::ArithRegReg>(inst).getDest();
  if (inst::Instruction::MUL <= op && op <= inst::Instruction::REMU)
    return spc<inst::MArith>(inst).getDest();
  return std::nullopt;
}

} // namespace

void Interpreter::interpret() {
  load();
  std::size_t numInsts = 0;

  std::stack<DebugStackFrame> debugStack;
  debugStack.emplace();

  try {
    while (pc != Interpretable::End) {
      if (!(0 <= pc && (std::uint32_t)pc < cache.storageSize())) {
        throw InvalidAddress(pc);
      }
      ++numInsts;
      if (numInsts > timeout) {
        throw Timeout("");
      }
      cache.tick();
      if (Interpretable::LibcFuncStart <= (std::uint32_t)pc &&
          (std::uint32_t)pc < Interpretable::LibcFuncEnd) {
        if (printInstructions) {
          std::cerr << "call libc-" << pc << std::endl;
        }
        if (keepDebugInfo) {
          debugStack.pop();
        }
        simulateLibCFunc(libc::Func(pc));
        if (printInstructions) {
          std::cerr << "\t\t# return value = " << regs.at(10) << std::endl;
        }
        pc = regs[1];
        // force the calling convention
        int callerSaved[] = {1,  5,  6,  7,  /* 10, */ 11, 12, 13, 14,
                             15, 16, 17, 28, 29,           30, 31};
        for (auto reg : callerSaved)
          regs[reg] += 0x1234;
        continue;
      }

      auto instIdx = *(std::uint32_t *)(cache.getMemory().first + pc);
      // We no longer take the mem/cache access in the IF stage into
      // consideration
      const auto &inst = interpretable.getInsts().at(instIdx);

      if (keepDebugInfo) {
        debugStack.top().addInstruction(inst);
        if (inst->getOp() == inst::Instruction::JALR) {
          // ret -> jalr x0, x1, 0
          const auto &jalr = spc<inst::JumpLinkReg>(inst);
          if (jalr.getDest() == 0 && jalr.getBase() == 1 &&
              jalr.getOffset() == 0)
            debugStack.pop();
          else
            debugStack.emplace();
        }
      }
      std::size_t modifiedReg = -1;
      std::uint64_t oldVal = -1;
      if (printInstructions) {
        printInstWithComment(inst);
        if (auto opt = getModifiedReg(inst)) {
          modifiedReg = opt.value();
          oldVal = regs.at(modifiedReg);
        }
      }

      simulate(inst);

      if (printInstructions) {
        if (modifiedReg != -1) {
          std::cerr << "\t\t# " << regNumber2regName(modifiedReg) << ": "
                    << oldVal << " -> " << regs.at(modifiedReg) << std::endl;
        } else if (inst::Instruction::SB <= inst->getOp() &&
                   inst->getOp() <= inst::Instruction::SW) {
          auto reg = spc<inst::MemAccess>(inst).getReg();
          std::cerr << "\t\t# stored value = " << regs.at(reg) << std::endl;
        }
      }

      regs[0] = 0;
      pc += 4;
    }
  } catch (std::exception &e) {
    if (!keepDebugInfo)
      throw;
    std::cerr << "\nSome error occurred.\n";
    std::cerr << "Printing the register state...";
    for (std::size_t i = 0; i < 32; ++i) {
      if (i % 8 == 0)
        std::cerr << "\n";
      std::cerr << std::setw(4) << regNumber2regName(i) << " = "
                << std::setw(11) << regs.at(i) << ",\t";
    }
    std::cerr << "\n\nPrinting the call stack...\n";
    while (!debugStack.empty()) {
      auto &frame = debugStack.top().lastFewInstructions;
      std::cerr << "\t...\n";
      while (!frame.empty()) {
        std::cerr << "\t";
        printInstWithComment(frame.front());
        frame.pop();
      }
      debugStack.pop();
      if (!debugStack.empty())
        std::cerr << "from ...\n";
    }
    std::cerr << std::endl;
    throw;
  }
}

void Interpreter::simulateLibCFunc(libc::Func funcN) {
  if (libc::PUTS <= funcN && funcN <= libc::PUTCHAR)
    ++instCnt.libcIO;
  else if (libc::MALLOC <= funcN && funcN <= libc::CALLOC) {
    // the actual update to instCnt is function-dependent. See the
    // implementations of each function for details
    ++instCnt.libcMem;
  } else
    assert(false);

  switch (funcN) {
  case libc::PUTS:
    libc::puts(regs, cache.getMemory().first, cache.getMemory().second, out);
    return;
  case libc::SCANF:
    libc::scanf(regs, cache.getMemory().first, cache.getMemory().second, in);
    return;
  case libc::SSCANF:
    libc::sscanf(regs, cache.getMemory().first);
  case libc::PRINTF:
    libc::printf(regs, cache.getMemory().first, cache.getMemory().second, out);
    return;
  case libc::SPRINTF:
    libc::sprintf(regs, cache.getMemory().first);
    return;
  case libc::PUTCHAR:
    libc::putchar(regs, out);
    return;
  case libc::MALLOC:
    libc::malloc(regs, cache.getMemory().first, cache.getMemory().second,
                 heapPtr, malloced, invalidAddress, instCnt.libcMem);
    return;
  case libc::CALLOC:
    libc::malloc(regs, cache.getMemory().first, cache.getMemory().second,
                 heapPtr, malloced, invalidAddress, instCnt.libcMem, true);
    return;
  case libc::FREE:
    libc::free(regs, malloced);
    return;
  case libc::MEMCPY:
    libc::memcpy(regs, cache.getMemory().first, cache.getMemory().second,
                 instCnt.libcMem);
    return;
  case libc::STRLEN:
    libc::strlen(regs, cache.getMemory().first);
    return;
  case libc::STRCPY:
    libc::strcpy(regs, cache.getMemory().first, cache.getMemory().second,
                 instCnt.libcMem);
    return;
  case libc::STRCAT:
    libc::strcat(regs, cache.getMemory().first, cache.getMemory().second,
                 instCnt.libcMem);
    return;
  case libc::STRCMP:
    libc::strcmp(regs, cache.getMemory().first);
    return;
  case libc::MEMSET:
    libc::memset(regs, cache.getMemory().first, cache.getMemory().second,
                 instCnt.libcMem);
    return;
  default:
    assert(false);
  }
  assert(false);
}

} // namespace ravel
