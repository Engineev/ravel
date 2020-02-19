#include <stdio.h>
#include <string.h>

#include "bultin.h"

int main() {
  char buffer[256] = {0};
  string_add(buffer, "Hello, ", "world!");
  puts(buffer);
  memset(buffer, 0, sizeof(buffer));
  string_substring(buffer, "Hello, world!\n", 1, 4);
  puts(buffer);
  return 0;
}