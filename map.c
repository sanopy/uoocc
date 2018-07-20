#include <stdlib.h>
#include <string.h>
#include "uoocc.h"

MapEntry *allocate_MapEntry(char *key, void *val) {
  MapEntry *e = (MapEntry *)malloc(sizeof(MapEntry));
  e->key = key;
  e->val = val;
  return e;
}

Map *map_new(void) {
  Map *m = (Map *)malloc(sizeof(Map));
  if (m == NULL)
    return NULL;
  m->vec = vector_new();
  m->size = 0;
  return m;
}

int map_put(Map *m, MapEntry *e) {
  if (vector_push_back(m->vec, e)) {
    m->size++;
    return 1;
  } else {
    return 0;
  }
}

MapEntry *map_get(Map *m, char *key) {
  for (int i = 0; i < m->size; i++) {
    MapEntry *e = (MapEntry *)vector_at(m->vec, i);
    if (strcmp(e->key, key) == 0)
      return e;
  }
  return NULL;
}
