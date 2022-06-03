#include "ravel/simulator.h"

#include <chrono>
#include <fstream>
#include <iostream>

#include "ravel/error.h"

namespace ravel {

Interpretable Simulator::buildInterpretable() {
  auto startTp = std::chrono::high_resolution_clock::now();

  std::vector<ObjectFile> objs;
  for (auto &src : config.sources)
    objs.emplace_back(assemble(src));

  auto buildEndTp = std::chrono::high_resolution_clock::now();
  auto time = std::chrono::duration_cast<std::chrono::milliseconds>(buildEndTp -
                                                                    startTp)
                  .count();
  auto interp = link(objs);
  std::cerr << "\nBuild finished in " << time << " ms\n";
  return interp;
}

std::pair<FILE *, FILE *> Simulator::getIOFile() const {
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

std::size_t Simulator::simulate() {
  auto interp = buildInterpretable();
  auto [in, out] = getIOFile();
  std::shared_ptr<void> close(nullptr, [in = in, out = out](void *) {
    if (in != stdin)
      std::fclose(in);
    if (out != stdout)
      std::fclose(out);
  });

  auto starTp = std::chrono::high_resolution_clock::now();

  auto regsPtr =
      regs.index() == 0 ? std::get<0>(regs).data() : std::get<1>(regs);
  auto storagePtr = storage.index() == 0
                        ? std::make_pair(&std::get<0>(storage).front(),
                                         &std::get<0>(storage).back() + 1)
                        : std::get<1>(storage);

  Interpreter interpreter{interp, regsPtr, storagePtr.first, storagePtr.second,
                          in,     out,     config.instWeight};

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

  return interpreter.getTimeConsumed();
}

Simulator::Simulator(Config config_) : config(std::move(config_)) {
  if (config.externalRegs) {
    regs = config.externalRegs;
  } else {
    regs = std::array<std::uint32_t, 32>();
  }

  if (config.externalStorageBegin) {
    assert(config.externalStorageBegin < config.externalStorageEnd);
    assert(config.externalStorageEnd - config.externalStorageBegin <
           config.maxStorageSize);
    storage =
        std::make_pair(config.externalStorageBegin, config.externalStorageEnd);
  } else {
    storage = std::vector<std::byte>(config.maxStorageSize);
  }
}

std::size_t simulate(const std::string &src, std::uint32_t *regs,
                     std::byte *storageBegin, std::byte *storageEnd) {
  Config config;
  config.cacheEnabled = true;
  config.sources.emplace_back(src);
  config.externalRegs = regs;
  config.externalStorageBegin = storageBegin;
  config.externalStorageEnd = storageEnd;

  Simulator simulator(config);
  auto timeConsumed = simulator.simulate();
  return timeConsumed;
}

} // namespace ravel