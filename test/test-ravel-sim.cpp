#include "ravel/ravel.h"

int main() {
  std::size_t size = 32 * 1024 * 1024;
  std::uint32_t regs[32];
  std::vector<std::byte> storage(size);

  // The function signature of ravel::simulate is
  // std::size_t simulate(const std::string &src, std::uint32_t *regs,
  //                     std::byte *storageBegin, std::byte *storageEnd);
  // The return value is the number of cycles.
  ravel::simulate(".text\n.globl main\nmain:\nnop\nret", regs, storage.data(),
                  storage.data() + size);

  return 0;
}