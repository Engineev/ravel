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

  std::string inputFile;
  std::string outputFile;
  std::vector<std::string> sources;
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