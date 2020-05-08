// by Yifan Xu, ACM2017

#include <stdio.h>
#include <stdlib.h>

struct Edge {
  int x;
  int y;
  int next;
};

int n;
int m;
int root;
int total = 0;
int MAX = 20;

int *first;
int *depth;
int *ans;
int **f;
int *pow2;
struct Edge **edge;

void init() {
  first = malloc(sizeof(int) * (n + 1));
  depth = malloc(sizeof(int) * (n + 1));
  int i;
  for (i = 0; i <= n; ++i) {
    first[i] = 0;
    depth[i] = 0;
  }
  ans = malloc(sizeof(int) * (m + 1));
  f = malloc(sizeof(int *) * (n + 1));
  for (i = 0; i <= n; ++i) {
    f[i] = malloc(sizeof(int) * (MAX + 1));
  }
  int j;
  for (i = 0; i <= n; ++i) {
    for (j = 0; j <= MAX; ++j) {
      f[i][j] = 0;
    }
  }
  pow2 = malloc(sizeof(int) * (MAX + 1));
  pow2[0] = 1;
  for (i = 1; i <= MAX; ++i) {
    pow2[i] = 2 * pow2[i - 1];
  }
  edge = malloc(sizeof(struct Edge *) * (2 * n - 1));
}

void addedge(int x, int y) {
  ++total;
  edge[total] = malloc(sizeof(struct Edge));
  edge[total]->x = x;
  edge[total]->y = y;
  edge[total]->next = first[x];
  first[x] = total;
}

void dfs(int x, int parent) {
  depth[x] = depth[parent] + 1;
  f[x][0] = parent;
  int i;
  for (i = 1; i <= MAX; ++i) {
    f[x][i] = f[f[x][i - 1]][i - 1];
  }
  for (i = first[x]; i != 0; i = edge[i]->next) {
    int y = edge[i]->y;
    if (y != parent) {
      dfs(y, x);
    }
  }
}

int lca(int a, int b) {
  if (depth[a] < depth[b]) {
    int tmp = a;
    a = b;
    b = tmp;
  }
  int i;
  for (i = MAX; i >= 0; --i) {
    if (depth[a] - depth[b] >= pow2[i]) {
      a = f[a][i];
    }
  }
  if (a == b) {
    return a;
  }
  for (i = MAX; i >= 0; --i) {
    if (f[a][i] != f[b][i]) {
      a = f[a][i];
      b = f[b][i];
    }
  }
  return f[a][0];
}

int main() {
  scanf("%d", &n);
  scanf("%d", &m);
  scanf("%d", &root);
  init();
  int i;
  for (i = 1; i <= n - 1; ++i) {
    int x;
    int y;
    scanf("%d", &x);
    scanf("%d", &y);
    addedge(x, y);
    addedge(y, x);
  }
  dfs(root, 0);
  for (i = 1; i <= m; ++i) {
    int a;
    int b;
    scanf("%d", &a);
    scanf("%d", &b);
    ans[i] = lca(a, b);
  }
  for (i = 1; i <= m; ++i) {
    printf("%d\n", ans[i]);
  }
  return 0;
}
