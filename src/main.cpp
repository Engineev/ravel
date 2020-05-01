#include <cassert>
#include <cstddef>
#include <iostream>
#include <unordered_map>

#include "assembler/parser.h"
#include "container_utils.h"
#include "simulator.h"

namespace ravel {

bool starts_with(const std::string &str, const std::string &prefix) {
  if (prefix.size() > str.size())
    return false;
  return prefix == str.substr(0, prefix.size());
}

class ArgParser {
public:
  explicit ArgParser(const std::vector<std::string> &args) : args(args) {}

  Config parse() {
    if (std::find(args.begin(), args.end(), "--oj-mode") != args.end()) {
      config.cacheEnabled = false;
      config.sources = {"test.s", "builtin.s"};
      config.inputFile = "test.in";
      config.outputFile = "test.out";
    }

    for (auto iter = args.begin() + 1; iter != args.end(); ++iter) {
      auto arg = *iter;
      if (arg == "--oj-mode")
        continue;

      if (arg.front() != '-') { // source code
        config.sources.emplace_back(arg);
        continue;
      }
      if (arg == "--enable-cache") {
        config.cacheEnabled = true;
        continue;
      }
      if (arg == "--keep-debug-info") {
        config.keepDebugInfo = true;
        continue;
      }
      if (starts_with(arg, "--timeout")) {
        auto tokens = split(arg, "=");
        config.timeout = std::stoul(tokens.at(1));
        continue;
      }
      if (starts_with(arg, "-w")) {
        handleInstWeight(arg);
        continue;
      }
      if (starts_with(arg, "--input-file")) {
        auto tokens = split(arg, "=");
        if (tokens.size() == 1)
          tokens.emplace_back("");
        config.inputFile = tokens.at(1);
        continue;
      }
      if (starts_with(arg, "--output-file")) {
        auto tokens = split(arg, "=");
        if (tokens.size() == 1)
          tokens.emplace_back("");
        config.outputFile = tokens.at(1);
        continue;
      }
      if (starts_with(arg, "--print-instructions")) {
        config.printInsts = true;
        continue;
      }
      std::cerr << "Unknown command line argument: " << arg << std::endl;
      exit(1);
    }
    return config;
  }

private:
  void handleInstWeight(const std::string &arg) {
    assert(starts_with(arg, "-w"));
    auto tokens = split(arg.substr(2), "=");
    std::size_t weight = std::stoul(tokens.at(1));
    auto type = tokens.at(0);
    if (type == "simple")
      config.instWeight.simple = weight;
    else if (type == "mul")
      config.instWeight.mul = weight;
    else if (type == "br")
      config.instWeight.br = weight;
    else if (type == "div")
      config.instWeight.div = weight;
    else if (type == "mem")
      config.instWeight.mem = weight;
    else if (type == "libcIO")
      config.instWeight.libcIO = weight;
    else if (type == "libcMem")
      config.instWeight.libcMem = weight;
    else
      assert(false);
  }

private:
  const std::vector<std::string> &args;
  Config config{};
};

} // namespace ravel

int main(int argc, char *argv[]) {
  using namespace ravel;
  std::vector<std::string> args;
  args.reserve(argc);
  for (int i = 0; i < argc; ++i)
    args.emplace_back(argv[i]);
  Config config = ArgParser(args).parse();

  Simulator simulator(config);
  simulator.simulate();

  return 0;
}