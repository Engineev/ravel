#include <fstream>
#include <iostream>
#include <unordered_map>

#include "assembler.h"
#include "instructions.h"
#include "interpreter.h"
#include "linker.h"

int main() {
  using namespace ravel;

  std::ifstream t(std::string(CMAKE_SOURCE_DIR) + "/test/c/asm/test.s");
  std::string src((std::istreambuf_iterator<char>(t)),
                  std::istreambuf_iterator<char>());
  auto obj = assemble(src);
  auto interp = link({obj});
  Interpreter interpreter{interp};
  interpreter.interpret();
  std::cout << "exit code: " << interpreter.getReturnCode() << std::endl;

  return 0;
}