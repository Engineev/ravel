#include <iostream>
#include <unordered_map>

#include "simulator.h"

namespace ravel {

Config parseArgs(const std::vector<std::string> &args) {
  Config config{};
  for (auto iter = args.begin() + 1; iter != args.end(); ++iter) {
    auto arg = *iter;
    if (arg.front() != '-') {
      config.sources.emplace_back(arg);
      continue;
    }
    if (arg == "--oj-mode") {
      config.sources = {"test.s", "builtin.s"};
      config.inputFile = "test.in";
      config.outputFile = "test.out";
      return config;
    }
    assert(false);
  }
  return config;
}

} // namespace ravel

int main(int argc, char *argv[]) {
  using namespace ravel;
  std::vector<std::string> args;
  args.reserve(argc);
  for (int i = 0; i < argc; ++i)
    args.emplace_back(argv[i]);
  Config config = parseArgs(args);

  Simulator simulator(config);
  simulator.simulate();

  return 0;
}