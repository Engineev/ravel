#pragma once

#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "assembler/assembler.h"
#include "interpreter/interpreter.h"
#include "linker/linker.h"

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
};

class Simulator {
public:
  explicit Simulator(Config config) : config(std::move(config)) {}

  void simulate();

private:
  Interpretable buildInterpretable();

  std::pair<FILE *, FILE *> getIOFile();

  void printResult(const Interpreter &interpreter) const;

private:
  Config config;
};

} // namespace ravel