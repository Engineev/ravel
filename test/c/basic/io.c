#include <stdio.h>

int global;

int main() {
  int local;
  scanf("%d", &local);
  printf("%d\n", local);

  scanf("%d", &global);
  printf("%d\n", global);

  scanf("%d%d", &local, &global);
  printf("%d %d\n", local, global);

  return 0;
}