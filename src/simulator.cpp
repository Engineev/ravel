#include "simulator.h"

#include <chrono>
#include <fstream>
#include <iostream>

#include "error.h"

namespace ravel {

Interpretable Simulator::buildInterpretable() {
  auto startTp = std::chrono::high_resolution_clock::now();

  std::vector<ObjectFile> objs;
  for (auto &file : config.sources) {
    std::ifstream t(file);
    if (!t) {
      throw Exception("Can not find file " + file);
    }
    std::string src((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    auto obj = assemble(src);
    objs.emplace_back(std::move(obj));
  }

  auto buildEndTp = std::chrono::high_resolution_clock::now();
  auto time = std::chrono::duration_cast<std::chrono::milliseconds>(buildEndTp -
                                                                    startTp)
                  .count();
  auto interp = link(objs);
  std::cerr << "\nBuild finished in " << time << " ms\n";
  return interp;
}

std::pair<FILE *, FILE *> Simulator::getIOFile() {
  auto in = config.inputFile.empty()
                ? stdin
                : std::fopen(config.inputFile.c_str(), "r");
  auto out = config.outputFile.empty()
                 ? stdout
                 : std::fopen(config.outputFile.c_str(), "w");
  assert(in && out);
  return {in, out};
}

void Simulator::printResult(const Interpreter &interpreter) const {
  std::cout << std::endl;
  std::cout << "exit code: " << interpreter.getReturnCode() << std::endl;
  std::cout << "memory leak: " << interpreter.hasMemoryLeak() << std::endl;
  std::cout << "time: " << interpreter.getTimeConsumed() << std::endl;
  std::cout << "# instructions:\n";
  auto iCnt = interpreter.getInstCnt();
  std::cout << "# simple  = " << iCnt.simple
            << " (including unconditional jump)\n";
  std::cout << "# mul     = " << iCnt.mul << std::endl;
  std::cout << "# cache   = " << iCnt.cache << std::endl;
  std::cout << "# br      = " << iCnt.br << std::endl;
  std::cout << "# div     = " << iCnt.div << std::endl;
  std::cout << "# mem     = " << iCnt.mem << " (a.k.a cache miss)" << std::endl;
  std::cout << "# libcIO  = " << iCnt.libcIO << std::endl;
  std::cout << "# libcMem = " << iCnt.libcMem << std::endl;
}

void Simulator::simulate() {
  auto interp = buildInterpretable();
  auto [in, out] = getIOFile();
  std::shared_ptr<void> close(nullptr, [in = in, out = out](void *) {
    if (in != stdin)
      std::fclose(in);
    if (out != stdout)
      std::fclose(out);
  });

  auto starTp = std::chrono::high_resolution_clock::now();
  Interpreter interpreter{interp, in, out, config.instWeight};
  interpreter.setTimeout(config.timeout);
  interpreter.setKeepDebugInfo(config.keepDebugInfo);
  if (!config.cacheEnabled)
    interpreter.disableCache();
  if (config.printInsts)
    interpreter.enablePrintInstructions();
  interpreter.interpret();
  printResult(interpreter);

  auto endTp = std::chrono::high_resolution_clock::now();
  auto time =
      std::chrono::duration_cast<std::chrono::milliseconds>(endTp - starTp)
          .count();
  std::cerr << "\nInterpretation finished in " << time << " ms\n";
}
} // namespace ravel