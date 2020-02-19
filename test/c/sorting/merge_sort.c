#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const size_t int_sz = sizeof(int);

void merge(int *buffer, int *l, size_t l_size, int *r, size_t r_size) {
  if (l_size == 0) {
    memcpy(buffer, r, r_size * int_sz);
    return;
  }
  if (r_size == 0) {
    memcpy(buffer, l, l_size * int_sz);
    return;
  }
  if (*l < *r) {
    *buffer = *l;
    merge(buffer + 1, l + 1, l_size - 1, r, r_size);
    return;
  }
  *buffer = *r;
  merge(buffer + 1, l, l_size, r + 1, r_size - 1);
}

void merge_sort(int *a, size_t n) {
  if (n == 1)
    return;
  size_t m = n / 2;
  merge_sort(a, m);
  merge_sort(a + m, n - m);
  int *buffer = malloc(sizeof(int) * n);
  merge(buffer, a, m, a + m, n - m);
  memcpy(a, buffer, n * int_sz);
  free(buffer);
}

int main() {
  int n;
  scanf("%d", &n);
  int *a = malloc(sizeof(int) * n);
  for (int i = 0; i < n; ++i)
    scanf("%d", &a[i]);

  merge_sort(a, n);

  for (int i = 0; i < n; ++i)
    printf("%d ", a[i]);
  printf("\n");

  free(a);
  return 0;
}