#include "bultin.h"

void string_add(char * buffer, char * str1, char * str2) {
  size_t len1 = strlen(str1);
  strcpy(buffer, str1);
  strcpy(buffer + len1, str2);
}

void string_substring(char * buffer, char * str, size_t l, size_t r) {
  memcpy(buffer, str + l, r - l);
}
