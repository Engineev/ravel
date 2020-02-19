#include <stdio.h>
#include <stdlib.h>

const int N = 100;

int main() {
  int *a = malloc(sizeof(int) * N);
  for (int i = 0; i < N; ++i)
    a[i] = i;
  for (int i = 0; i < N; ++i)
    printf("%d", a[i]);
  free(a);
  return 0;
}