// By Yunwei Ren, ACM 2017

#include <stdio.h>
#include <stdlib.h>

struct Edge {
  int u, v, w;
};

int n, m;
struct Edge **edges;
struct Edge **buffer;

// disjoint set
int *pnt;

void init() {
  scanf("%d%d", &n, &m);
  edges = malloc(sizeof(struct Edge *) * m);
  buffer = malloc(sizeof(struct Edge *) * m);
  for (int i = 0; i < m; ++i) {
    edges[i] = malloc(sizeof(struct Edge));
    scanf("%d%d%d", &edges[i]->u, &edges[i]->v, &edges[i]->w);
  }

  pnt = malloc(sizeof(int) * n);
  for (int i = 0; i < n; ++i)
    pnt[i] = -1;
}

void sort(int l, int r) {
  if (l + 1 == r)
    return;
  int mid = (l + r) / 2;
  sort(l, mid);
  sort(mid, r);

  int lpos = l;
  int rpos = mid;
  for (int i = 0; i < r - l; ++i) {
    if (lpos == mid) {
      buffer[i] = edges[rpos++];
      continue;
    }
    if (rpos == r) {
      buffer[i] = edges[lpos++];
      continue;
    }
    if (edges[lpos]->w < edges[rpos]->w) {
      buffer[i] = edges[lpos++];
      continue;
    }
    buffer[i] = edges[rpos++];
  }

  for (int i = 0; i < r - l; ++i) {
    edges[l + i] = buffer[i];
  }
}

int findRoot(int x) {
  if (pnt[x] == -1)
    return x;
  int y = findRoot(pnt[x]);
  pnt[x] = y;
  return y;
}

void unionSets(int x, int y) {
  int px = findRoot(x);
  int py = findRoot(y);
  if (px == py)
    return;
  pnt[px] = py;
}

int main() {
  init();
  sort(0, m);

  int cnt = 0;
  int ans = 0;
  for (int i = 0; i < m /* && cnt != n - 1 */; ++i) {
    if (findRoot(edges[i]->u) == findRoot(edges[i]->v))
      continue;
    unionSets(edges[i]->u, edges[i]->v);
    ++cnt;
    ans += edges[i]->w;
  }
  printf("%d\n", ans);
  return 0;
}