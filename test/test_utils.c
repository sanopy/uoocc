#include <assert.h>
#include <stdlib.h>
#include "../uoocc.h"

void test_vector(void) {
  Vector *v = vector_new();

  assert(v != NULL);

  int times = 64;
  for (int i = 0; i < times; i++) {
    assert(vector_push_back(v, (void *)allocate_integer(i)) == 1);
    assert(v->size == i + 1);
  }

  for (int i = 0; i < times; i++) {
    int *p = (int *)vector_at(v, i);
    assert(*p == i);
  }

  assert(vector_at(v, -1) == NULL);
  assert(vector_at(v, times) == NULL);
}

void test_map(void) {
  MapEntry *e;
  Map *m = map_new(NULL);

  assert(m != NULL);

  e = allocate_MapEntry(allocate_string("abc"), (void *)allocate_integer(1));
  assert(map_put(m, e) == 1);
  e = allocate_MapEntry(allocate_string("xyz"), (void *)allocate_integer(2));
  assert(map_put(m, e) == 1);
  e = allocate_MapEntry(allocate_string("a"), (void *)allocate_integer(3));
  assert(map_put(m, e) == 1);
  e = allocate_MapEntry(allocate_string("123"), (void *)allocate_integer(4));
  assert(map_put(m, e) == 1);

  assert(*(int *)(map_get(m, "abc")->val) == 1);
  assert(*(int *)(map_get(m, "xyz")->val) == 2);
  assert(*(int *)(map_get(m, "a")->val) == 3);
  assert(*(int *)(map_get(m, "123")->val) == 4);

  assert(map_get(m, "hoge") == NULL);
}

int main(void) {
  test_vector();
  test_map();

  return 0;
}
