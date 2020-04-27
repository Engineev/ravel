#include <stdio.h>
#include <stdlib.h>

int MAXK = 105;
int MAXN = 100005;

int main() {
  int point = 0;
  int k;
  int MIN;
  int K;
  int N;
  int i;
  int *primes = malloc(sizeof(int) * MAXK);
  int *pindex = malloc(sizeof(int) * MAXK);
  int *ans = malloc(sizeof(int) * MAXN);
  scanf("%d %d", &K, &N);
  for (i = 0; i < K; ++i) {
    scanf("%d", &primes[i]);
  }
  ans[0] = 1;
  while (point <= N) {
    MIN = 2139062143;
    for (i = 0; i < K; ++i) {
      while (ans[point] >= primes[i] * ans[pindex[i]])
        pindex[i]++;
      if (primes[i] * ans[pindex[i]] < MIN) {
        MIN = primes[i] * ans[pindex[i]];
        k = i;
      }
    }
    ans[++point] = MIN;
  }
  printf("%d\n", ans[N]);
  return 0;
}
