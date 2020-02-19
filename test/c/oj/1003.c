// https://acm.sjtu.edu.cn/OnlineJudge/problem/1003

#include <stdio.h>

int l;
int dish[100][100];

int isFinished() {
  for (int i = 0; i < l; ++i) {
    for (int j = 0; j < l; ++j) {
      if (!dish[i][j])
        return 0;
    }
  }
  return 1;
}

void step() {
  int kx[4] = {1, -1, 0, 0};
  int ky[4] = {0, 0, 1, -1};

  for (int i = 0; i < l; ++i) {
    for (int j = 0; j < l; ++j) {
      if (dish[i][j] != 1)
        continue;
      for (int k = 0; k < 4; ++k) {
        int nx = i + kx[k], ny = j + ky[k];
        if (nx < 0 || ny < 0 || nx == l || ny == l)
          continue;
        if (dish[nx][ny] == 0)
          dish[nx][ny] = -1;
      }
    }
  }

  for (int i = 0; i < l; ++i) {
    for (int j = 0; j < l; ++j) {
      if (dish[i][j] == -1)
        dish[i][j] = 1;
    }
  }
}

int main() {
  scanf("%d", &l);

  for (int i = 0; i < l; ++i) {
    for (int j = 0; j < l; ++j) {
      scanf("%d", &dish[i][j]);
    }
  }

  int rounds = 0;
  while (!isFinished()) {
    step();
    ++rounds;
  }

  printf("%d\n", rounds);

  return 0;
}