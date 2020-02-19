#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

void selection_sort(int *a, int n) {
  for (int i = 0; i < n - 1; ++i) {
    int min_pos = i;
    for (int j = i + 1; j < n; ++j) {
      if (a[j] < a[min_pos])
        min_pos = j;
    }
    int t = a[i];
    a[i] = a[min_pos];
    a[min_pos] = t;
  }
}

int main() {
  int n;
  scanf("%d", &n);
  int *a = malloc(sizeof(int) * n);
  for (int i = 0; i < n; ++i)
    scanf("%d", &a[i]);

  selection_sort(a, n);

  for (int i = 0; i < n; ++i)
    printf("%d ", a[i]);
  printf("\n");

  free(a);
  return 0;
}