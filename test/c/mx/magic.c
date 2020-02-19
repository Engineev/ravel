#include <stdio.h>
#include <stdlib.h>

#define N 3

int make[N][N];
int color[10];
int count[1];
int i;
int j;

void origin() {
  for (i = 0; i < N; i++) {
    for (j = 0; j < N; j++)
      make[i][j] = 0;
  }
}

int search(int x, int y, int z) {
  int s;
  int i;
  int j;
  if ((y > 0 || y < 0) || x == 0 ||
      make[x - 1][0] + make[x - 1][1] + make[x - 1][2] == 15) {
    if (x == 2 && y == 2) {
      make[2][2] = 45 - z;
      s = make[0][0] + make[0][1] + make[0][2];
      if (make[1][0] + make[1][1] + make[1][2] == s &&
          make[2][0] + make[2][1] + make[2][2] == s &&
          make[0][0] + make[1][0] + make[2][0] == s &&
          make[0][1] + make[1][1] + make[2][1] == s &&
          make[0][2] + make[1][2] + make[2][2] == s &&
          make[0][0] + make[1][1] + make[2][2] == s &&
          make[2][0] + make[1][1] + make[0][2] == s) {
        count[0] = count[0] + 1;
        for (i = 0; i <= 2; i++) {
          for (j = 0; j <= 2; j++) {
            printf("%d", make[i][j]);
            printf(" ");
          }
          printf("\n");
        }
        printf("\n");
      }
    } else {
      if (y == 2) {
        make[x][y] = 15 - make[x][0] - make[x][1];
        if (make[x][y] > 0 && make[x][y] < 10 && color[make[x][y]] == 0) {
          color[make[x][y]] = 1;
          if (y == 2)
            search(x + 1, 0, z + make[x][y]);
          else
            search(x, y + 1, z + make[x][y]);
          color[make[x][y]] = 0;
        }
      } else {
        for (i = 1; i <= 9; i++) {
          if (color[i] == 0) {
            color[i] = 1;
            make[x][y] = i;
            if (y == 2)
              search(x + 1, 0, z + i);
            else
              search(x, y + 1, z + i);
            make[x][y] = 0;
            color[i] = 0;
          }
        }
      }
    }
  }
}

int main() {
  count[0] = 0;
  origin();
  search(0, 0, 0);
  printf("%d", count[0]);
  return 0;
}

/*!! metadata:
=== comment ===
magic-5100309153-yanghuan.mx
=== input ===

=== assert ===
output
=== timeout ===
0.1
=== output ===
2 7 6
9 5 1
4 3 8

2 9 4
7 5 3
6 1 8

4 3 8
9 5 1
2 7 6

4 9 2
3 5 7
8 1 6

6 1 8
7 5 3
2 9 4

6 7 2
1 5 9
8 3 4

8 1 6
3 5 7
4 9 2

8 3 4
1 5 9
6 7 2

8
=== phase ===
codegen pretest
=== is_public ===
True

!!*/
