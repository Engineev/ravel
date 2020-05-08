// By Yunwei Ren, ACM 2017
// binary search tree
//

#include <stdio.h>
#include <stdlib.h>
//#include <assert.h>

struct Node {
  struct Node *pnt;
  struct Node **children;
  int key;
  int duplicate;
};

struct Node *constructNode(int key, struct Node *pnt, struct Node *lchild,
                           struct Node *rchild) {
  struct Node *node = malloc(sizeof(struct Node));
  node->children = malloc(sizeof(struct Node *) * 2);
  node->key = key;
  node->duplicate = 1;
  node->pnt = pnt;
  node->children[0] = lchild;
  node->children[1] = rchild;
  return node;
}

struct Node *root = NULL;

int insertImpl(struct Node *cur, struct Node *pnt, int childId, int key) {
  if (cur == NULL) {
    cur = constructNode(key, pnt, NULL, NULL);
    pnt->children[childId] = cur;
    return 0;
  }
  if (cur->key == key) {
    ++cur->duplicate;
    return 1;
  }
  int id = 0;
  if (cur->key < key)
    id = 1;
  return insertImpl(cur->children[id], cur, id, key);
}

// return 1 if isIn
int insert(int key) {
  if (root != NULL)
    return insertImpl(root, NULL, -1, key);
  root = constructNode(key, NULL, NULL, NULL);
  return 0;
}

struct Node *findLargest(struct Node *cur) {
  if (cur->children[1] == NULL)
    return cur;
  return findLargest(cur->children[1]);
}

int eraseImpl(struct Node *cur, struct Node *pnt, int childId, int key) {
  if (cur == NULL)
    return 0;
  if (cur->key > key)
    return eraseImpl(cur->children[0], cur, 0, key);
  if (cur->key < key)
    return eraseImpl(cur->children[1], cur, 1, key);
  --cur->duplicate;
  if (cur->duplicate > 0)
    return 1;
  //  assert(cur->duplicate == 0);
  if (cur->children[0] == NULL) {
    if (pnt != NULL)
      pnt->children[childId] = cur->children[1];
    if (cur->children[1] != NULL)
      cur->children[1]->pnt = pnt;
    if (key == root->key)
      root = cur->children[1];
    return 1;
  }
  struct Node *replacement = findLargest(cur->children[0]);
  if (key == root->key)
    root = replacement;
  //  assert(replacement->children[1] == NULL);
  if (replacement->key != cur->children[0]->key) {
    replacement->pnt->children[1] = replacement->children[0];
    if (replacement->children[0] != NULL)
      replacement->children[0]->pnt = replacement->pnt;
  }
  if (pnt != NULL)
    pnt->children[childId] = replacement;
  replacement->pnt = pnt;
  replacement->children[1] = cur->children[1];
  if (cur->children[1] != NULL)
    cur->children[1]->pnt = replacement;
  if (replacement->key != cur->children[0]->key) {
    replacement->children[0] = cur->children[0];
    cur->children[0]->pnt = replacement;
  }
  return 1;
}

// return 1 if isIn
int erase(int key) {
  if (root == NULL)
    return 0;
  return eraseImpl(root, NULL, -1, key);
}

void print(struct Node *cur) {
  if (cur == NULL)
    return;
  print(cur->children[0]);
  printf("%d: %d\n", cur->key, cur->duplicate);
  print(cur->children[1]);
}

int main() {
  while (1) {
    int type, value;
    scanf("%d", &type);
    if (type == 0)
      break;
    scanf("%d", &value);
    if (type == 1) {
      insert(value);
    } else {
      erase(value);
    }
  }
  print(root);
  return 0;
}