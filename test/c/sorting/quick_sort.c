#include <stdio.h>
#include <stdlib.h>

void swap(int *a, int *b) {
  int t = *a;
  *a = *b;
  *b = t;
}

int partition(int *a, int p, int r) {
  int pivot = a[r];
  int i = p - 1;
  for (int j = p; j < r; ++j) {
    if (a[j] <= pivot) {
      ++i;
      swap(a + i, a + j);
    }
  }
  swap(a + i + 1, a + r);
  return i + 1;
}

void quick_sort(int *a, int p, int r) {
  if (p >= r)
    return;
  int q = partition(a, p, r);
  quick_sort(a, p, q - 1);
  quick_sort(a, q + 1, r);
}

int main() {
  int n;
  scanf("%d", &n);
  int *a = malloc(sizeof(int) * n);
  for (int i = 0; i < n; ++i)
    scanf("%d", &a[i]);

  quick_sort(a, 0, n - 1);

  for (int i = 0; i < n; ++i)
    printf("%d ", a[i]);
  printf("\n");

  free(a);
  return 0;
}