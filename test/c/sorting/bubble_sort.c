#include <stdio.h>
#include <stdlib.h>

void bubble_sort(int *a, int n) {
  for (int i = 0; i < n - 1; ++i) {
    for (int j = 0; j < n - i - 1; ++j) {
      if (a[j] > a[j + 1]) {
        int t = a[j];
        a[j] = a[j + 1];
        a[j + 1] = t;
      }
    }
  }
}

int main() {
  int n;
  scanf("%d", &n);
  int *a = malloc(sizeof(int) * n);
  for (int i = 0; i < n; ++i)
    scanf("%d", &a[i]);

  bubble_sort(a, n);

  for (int i = 0; i < n; ++i)
    printf("%d ", a[i]);
  printf("\n");

  free(a);

  return 0;
}