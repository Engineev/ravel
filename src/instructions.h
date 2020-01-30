#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <string>

namespace ravel::inst {

class Instruction {
public:
  // clang-format off
  enum OpType {
    LUI, AUIPC,
    JAL, JALR,
    BEQ, BNE, BLT, BGE, BLTU, BGEU,
    LB, LH, LW, LBU, LHU, SB, SH, SW,
    ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI,
    ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND,
    // TODO: fence ...
  };
  // clang-format on

  class Id {
    template <class K> friend struct std::hash;

  public:
    Id() {
      static int currentId = 1;
      val = currentId++;
    }

    bool operator==(const Id &rhs) const { return val == rhs.val; }

    bool operator!=(const Id &rhs) const { return !(*this == rhs); }

  private:
    int val;
  };

public:
  explicit Instruction(OpType type) : op(type) {}
  virtual ~Instruction() = default;

  OpType getOp() const { return op; }

  const Id &getId() const { return id; }

private:
  Id id;
  OpType op;
};

} // namespace ravel::inst

namespace ravel::inst {

class ImmConstruction : public Instruction {
public:
  ImmConstruction(OpType op, std::size_t dest, int imm)
      : Instruction(op), dest(dest), imm(imm) {}

  size_t getDest() const { return dest; }
  int getImm() const { return imm; }

private:
  std::size_t dest;
  int imm;
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
  JumpLink(std::size_t dest, int offset)
      : Instruction(JAL), dest(dest), offset(offset) {}

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
  Branch(OpType op, std::size_t src1, std::size_t src2, int offset)
      : Instruction(op), src1(src1), src2(src2), offset(offset) {}

  size_t getSrc1() const { return src1; }
  size_t getSrc2() const { return src2; }
  int getOffset() const { return offset; }

private:
  std::size_t src1, src2;
  int offset;
};

} // namespace ravel::inst

namespace std {
template <> struct hash<ravel::inst::Instruction::Id> {
  using Key = ravel::inst::Instruction::Id;

  std::size_t operator()(const Key &k) const { return std::hash<int>()(k.val); }
};
} // namespace std
