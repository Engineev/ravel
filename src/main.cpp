#include <chrono>
#include <fstream>
#include <iostream>
#include <unordered_map>

#include "argparse.h"
#include "assembler.h"
#include "interpreter.h"
#include "linker.h"

int main(int argc, char *argv[]) {
  using namespace ravel;

  auto startTp = std::chrono::high_resolution_clock::now();

  std::vector<std::string> args;
  for (int i = 0; i < argc; ++i)
    args.emplace_back(argv[i]);
  Config config = parseArgs(args);

  std::vector<ObjectFile> objs;
  for (auto &file : config.files) {
    std::ifstream t(file);
    std::string src((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    auto obj = assemble(src);
    objs.emplace_back(std::move(obj));
  }

  auto buildEndTp = std::chrono::high_resolution_clock::now();
  auto time = std::chrono::duration_cast<std::chrono::milliseconds>(buildEndTp -
                                                                    startTp)
                  .count();
  std::cerr << "\nBuild finished in " << time << " ms\n";

  auto interp = link(objs);
  Interpreter interpreter{interp};
  interpreter.interpret();
  std::cerr << "\nProcess finished with exit code "
            << interpreter.getReturnCode() << std::endl;
  auto endTp = std::chrono::high_resolution_clock::now();
  time =
      std::chrono::duration_cast<std::chrono::milliseconds>(endTp - buildEndTp)
          .count();
  std::cerr << "\nInterpretation finished in " << time << " ms\n";

  return 0;
}