/*
input:
1 2 3 4
Hello, world 5 6!
output:
1
2
3 4
Hello, world 11!
*/

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

  scanf("Hello, world %d %d!", &local, &global);
  printf("Hello, world %d!", local + global);

  return 0;
}