#include <stdlib.h>
#include "uoocc.h"

Vector *vector_new(void) {
  Vector *v = (Vector *)malloc(sizeof(Vector));
  if (v == NULL)
    return NULL;
  v->size = 0;
  v->reserved_size = VECTOR_BLOCK_SIZE;
  v->data = (void **)malloc(v->reserved_size * sizeof(void *));
  if (v->data == NULL)
    return NULL;
  return v;
}

int vector_realloc(Vector *v, int new_size) {
  void **p = realloc(v->data, new_size * sizeof(void *));
  if (p == NULL)
    return 0;
  v->data = p;
  v->reserved_size = new_size;
  return 1;
}

int vector_push_back(Vector *v, void *data) {
  if (v->reserved_size <= v->size) {
    int res = vector_realloc(v, v->reserved_size + VECTOR_BLOCK_SIZE);
    if (res == 0)
      return 0;
  }
  v->data[v->size++] = data;
  return 1;
}

void *vector_at(Vector *v, int pos) {
  if (pos < 0 || v->size <= pos)
    return NULL;
  else
    return v->data[pos];
}
