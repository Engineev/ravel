#pragma once

#include <array>
#include <cstdio>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "ravel/assembler/assembler.h"
#include "ravel/interpreter/interpreter.h"
#include "ravel/linker/linker.h"

namespace ravel {

struct Config {
  Config() = default;

  bool printInsts = false;
  bool cacheEnabled = false;
  bool keepDebugInfo = false;
  std::string inputFile;
  std::string outputFile;
  std::vector<std::string> sources;
  InstWeight instWeight = InstWeight();
  // exits when # of instructions executed exceeds `timeout`
  std::size_t timeout = (std::size_t)-1;

  std::size_t maxStorageSize = 512 * 1024 * 1024;

  // use external registers and memory
  std::uint32_t *externalRegs = nullptr;
  std::byte *externalStorageBegin = nullptr;
  std::byte *externalStorageEnd = nullptr;
};

class Simulator {
public:
  explicit Simulator(Config config_);

  std::size_t simulate();

private:
  Interpretable buildInterpretable();

  std::pair<FILE *, FILE *> getIOFile() const;

  void printResult(const Interpreter &interpreter) const;

private:
  Config config;
  std::variant<std::array<std::uint32_t, 32>, std::uint32_t *> regs;
  std::variant<std::vector<std::byte>, std::pair<std::byte *, std::byte *>>
      storage;
};

} // namespace ravel

namespace ravel {

std::size_t simulate(const std::string &src, std::uint32_t *regs,
                     std::byte *storageBegin, std::byte *storageEnd);

}
