#include <stdio.h>
#include <stdlib.h>

struct Edge {
  int from;
  int to;
  int weight;
};

struct EdgeList {
  struct Edge **edges;
  int *next;
  int *first;
  int size;
};

void EdgeList_init(struct EdgeList *g, int n, int m) {
  g->edges = malloc(sizeof(struct Edge *) * m);
  g->next = malloc(sizeof(int) * m);
  g->first = malloc(sizeof(int) * n);
  int i;
  for (i = 0; i < m; ++i)
    g->next[i] = -1;
  for (i = 0; i < n; ++i)
    g->first[i] = -1;
  g->size = 0;
}

void EdgeList_addEdge(struct EdgeList *g, int u, int v, int w) {
  struct Edge *e = malloc(sizeof(struct Edge));
  e->from = u;
  e->to = v;
  e->weight = w;

  g->edges[g->size] = e;
  g->next[g->size] = g->first[u];
  g->first[u] = g->size;
  ++g->size;
}

struct Node {

  int node;
  int dist;
};

int Node_key_(struct Node *this) { return -this->dist; }

struct Array_Node {
  struct Node **storage;
  int sz;
  int capacity;
};

void Array_Node_Ctor(struct Array_Node *this);
void Array_Node_push_back(struct Array_Node *this, struct Node *v);
struct Node *Array_Node_pop_back(struct Array_Node *this);
struct Node *Array_Node_back(struct Array_Node *this);
struct Node *Array_Node_front(struct Array_Node *this);
int Array_Node_size(struct Array_Node *this);
void Array_Node_resize(struct Array_Node *this, int newSize);
struct Node *Array_Node_get(struct Array_Node *this, int i);
void Array_Node_set(struct Array_Node *this, int i, struct Node *v);
void Array_Node_swap(struct Array_Node *this, int i, int j);
void Array_Node_doubleStorage(struct Array_Node *this);

struct Array_Node *heap;
void init_heap();
void push(struct Node *v);
struct Node *pop();
struct Node *top();
int size();
int lchild(int x);
int rchild(int x);
int pnt(int x);
void maxHeapify(int x);

int n;
int m;
struct EdgeList *g;
int INF = 10000000;

void init() {
  scanf("%d %d", &n, &m);
  g = malloc(sizeof(struct EdgeList));
  EdgeList_init(g, n, m);

  init_heap();

  int i;
  for (i = 0; i < m; ++i) {
    int u, v, w;
    scanf("%d %d %d", &u, &v, &w);
    EdgeList_addEdge(g, u, v, w);
  }
}

int *dijkstra(int s) {
  int *visited = malloc(sizeof(int) * n);
  int *d = malloc(sizeof(int) * n);
  int i;
  for (i = 0; i < n; ++i) {
    d[i] = INF;
    visited[i] = 0;
  }
  d[s] = 0;

  init_heap();
  struct Node *src = malloc(sizeof(struct Node));
  src->dist = 0;
  src->node = s;
  push(src);

  while (size() != 0) {
    struct Node *node = pop();
    int u = node->node;
    if (visited[u] == 1)
      continue;
    visited[u] = 1;
    int k;
    for (k = g->first[u]; k != -1; k = g->next[k]) {
      int v = g->edges[k]->to;
      int w = g->edges[k]->weight;
      int alt = d[u] + w;
      if (alt >= d[v])
        continue;
      d[v] = alt;
      node = malloc(sizeof(struct Node));
      node->node = v;
      node->dist = d[v];
      push(node);
    }
  }

  return d;
}

int main() {
  init();
  int i;
  int j;
  for (i = 0; i < n; ++i) {
    int *d = dijkstra(i);
    for (j = 0; j < n; ++j) {
      if (d[j] == INF) {
        printf("-1");
      } else {
        printf("%d", d[j]);
      }
      printf(" ");
    }
    puts("");
  }

  return 0;
}

void Array_Node_Ctor(struct Array_Node *this) {
  this->sz = 0;
  this->capacity = 16;
  this->storage = malloc(sizeof(struct Node *) * this->capacity);
}

void Array_Node_push_back(struct Array_Node *this, struct Node *v) {
  if (Array_Node_size(this) == this->capacity) {
    Array_Node_doubleStorage(this);
  }
  this->storage[this->sz] = v;
  ++this->sz;
}

struct Node *Array_Node_pop_back(struct Array_Node *this) {
  --this->sz;
  return this->storage[this->sz];
}

struct Node *Array_Node_back(struct Array_Node *this) {
  return this->storage[this->sz - 1];
}

struct Node *Array_Node_front(struct Array_Node *this) {
  return this->storage[0];
}

int Array_Node_size(struct Array_Node *this) { return this->sz; }

void Array_Node_resize(struct Array_Node *this, int newSize) {
  while (this->capacity < newSize)
    Array_Node_doubleStorage(this);
  this->sz = newSize;
}

struct Node *Array_Node_get(struct Array_Node *this, int i) {
  return this->storage[i];
}

void Array_Node_set(struct Array_Node *this, int i, struct Node *v) {
  this->storage[i] = v;
}

void Array_Node_swap(struct Array_Node *this, int i, int j) {
  struct Node *t = this->storage[i];
  this->storage[i] = this->storage[j];
  this->storage[j] = t;
}

void Array_Node_doubleStorage(struct Array_Node *this) {
  this->capacity *= 2;

  struct Node **copy = this->storage;
  int szCopy = this->sz;

  this->storage = malloc(sizeof(struct Node *) * this->capacity);
  this->sz = 0;

  for (; this->sz != szCopy; ++this->sz) {
    this->storage[this->sz] = copy[this->sz];
  }
}

void init_heap() {
  heap = malloc(sizeof(struct Array_Node));
  Array_Node_Ctor(heap);
}

void push(struct Node *v) {
  Array_Node_push_back(heap, v);
  int x = Array_Node_size(heap) - 1;
  while (x > 0) {
    int p = pnt(x);
    if (Node_key_(Array_Node_get(heap, p)) >=
        Node_key_(Array_Node_get(heap, x)))
      break;
    Array_Node_swap(heap, p, x);
    x = p;
  }
}

struct Node *pop() {
  struct Node *res = Array_Node_front(heap);
  Array_Node_swap(heap, 0, Array_Node_size(heap) - 1);
  Array_Node_pop_back(heap);
  maxHeapify(0);
  return res;
}

struct Node *top() {
  return Array_Node_front(heap);
}

int size() { return Array_Node_size(heap); }

// private:
int lchild(int x) { return x * 2 + 1; }

int rchild(int x) { return x * 2 + 2; }

int pnt(int x) { return (x - 1) / 2; }

void maxHeapify(int x) {
  int l = lchild(x);
  int r = rchild(x);
  int largest = x;

  if (l < size() && Node_key_(Array_Node_get(heap, l)) >
      Node_key_(Array_Node_get(heap, largest)))
    largest = l;
  if (r < size() && Node_key_(Array_Node_get(heap, r)) >
      Node_key_(Array_Node_get(heap, largest)))
    largest = r;

  if (largest == x)
    return;
  Array_Node_swap(heap, x, largest);
  maxHeapify(largest);
}