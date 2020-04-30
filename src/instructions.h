#pragma once

#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>

#include "container_utils.h"

namespace ravel::inst {

class Instruction {
public:
  // clang-format off
  enum OpType {
    LUI, AUIPC,  // ImmConstruction
    JAL,         // JumpLink
    JALR,        // JumpLinkReg
    BEQ, BNE, BLT, BGE, BLTU, BGEU,                        // Branch
    LB, LH, LW, LBU, LHU, SB, SH, SW,                      // MemAccess
    ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI,  // ArithRegImm
    ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND,      // ArithRegReg
    // TODO: fence ...
    MUL, MULH, MULHSU, MULHU, DIV, DIVU, REM, REMU,  // MArith
  };
  // clang-format on

  using Id = ravel::Id<Instruction>;

public:
  explicit Instruction(OpType type) : op(type) {}
  Instruction(OpType type, std::string comment)
      : op(type), comment(std::move(comment)) {}
  virtual ~Instruction() = default;

  OpType getOp() const { return op; }

  const Id &getId() const { return id; }

  const std::string &getComment() const { return comment; }

private:
  Id id;
  OpType op;
  std::string comment;
};

} // namespace ravel::inst

namespace ravel::inst {

class ImmConstruction : public Instruction {
public:
  ImmConstruction(OpType op, std::size_t dest, std::uint32_t imm)
      : Instruction(op), dest(dest), imm(imm) {
    assert((imm >> 20u) == 0 || ((-imm) >> 20u) == 0);
  }

  size_t getDest() const { return dest; }
  std::uint32_t getImm() const { return imm; }

private:
  std::size_t dest;
  std::uint32_t imm;
};

class ArithRegReg : public Instruction {
public:
  ArithRegReg(OpType op, std::size_t dest, std::size_t src1, std::size_t src2)
      : Instruction(op), dest(dest), src1(src1), src2(src2) {}

  size_t getDest() const { return dest; }
  size_t getSrc1() const { return src1; }
  size_t getSrc2() const { return src2; }

private:
  std::size_t dest, src1, src2;
};

class ArithRegImm : public Instruction {
public:
  ArithRegImm(OpType type, size_t dest, size_t src, int imm)
      : Instruction(type), dest(dest), src(src), imm(imm) {}

  size_t getDest() const { return dest; }

  size_t getSrc() const { return src; }

  int getImm() const { return imm; }

private:
  std::size_t dest, src;
  int imm;
};

class MemAccess : public Instruction {
public:
  MemAccess(OpType type, std::size_t reg, std::size_t base, int offset)
      : Instruction(type), reg(reg), base(base), offset(offset) {}

  size_t getReg() const { return reg; }

  size_t getBase() const { return base; }

  int getOffset() const { return offset; }

private:
  std::size_t reg, base;
  int offset;
};

class JumpLink : public Instruction {
public:
  JumpLink(std::size_t dest, int offset, std::string comment = "")
      : Instruction(JAL, std::move(comment)), dest(dest), offset(offset) {}

  size_t getDest() const { return dest; }
  int getOffset() const { return offset; }

private:
  std::size_t dest;
  int offset;
};

class JumpLinkReg : public Instruction {
public:
  JumpLinkReg(std::size_t dest, std::size_t base, int offset)
      : Instruction(JALR), dest(dest), base(base), offset(offset) {}

  size_t getDest() const { return dest; }
  size_t getBase() const { return base; }
  int getOffset() const { return offset; }

private:
  std::size_t dest;
  std::size_t base;
  int offset;
};

class Branch : public Instruction {
public:
  Branch(OpType op, std::size_t src1, std::size_t src2, int offset,
         std::string comment = "")
      : Instruction(op, std::move(comment)), src1(src1), src2(src2),
        offset(offset) {}

  size_t getSrc1() const { return src1; }
  size_t getSrc2() const { return src2; }
  int getOffset() const { return offset; }

private:
  std::size_t src1, src2;
  int offset;
};

class MArith : public Instruction {
public:
  MArith(Instruction::OpType op, std::size_t dest, std::size_t src1,
         std::size_t src2)
      : Instruction(op), dest(dest), src1(src1), src2(src2) {}

  size_t getDest() const { return dest; }
  size_t getSrc1() const { return src1; }
  size_t getSrc2() const { return src2; }

private:
  std::size_t dest, src1, src2;
};

} // namespace ravel::inst
