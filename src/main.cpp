#include <fstream>
#include <iostream>
#include <unordered_map>

#include "assembler.h"
#include "instructions.h"
#include "linker.h"

int main() {
  using namespace ravel;

  std::ifstream t(std::string(CMAKE_SOURCE_DIR) + "/test/asm/a_plus_b_func.s");
  std::string src((std::istreambuf_iterator<char>(t)),
                  std::istreambuf_iterator<char>());
  auto obj = assemble(src);
  auto interp = link({obj});

  return 0;
}