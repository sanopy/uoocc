#include <assert.h>
#include <stdlib.h>
#include "../uoocc.h"

void test_vector(void) {
  Vector *v = vector_new();

  assert(v != NULL);

  int times = 64;
  for (int i = 0; i < times; i++) {
    int *p = malloc(sizeof(int));
    *p = i;
    assert(vector_push_back(v, p) == 1);
  }

  for (int i = 0; i < times; i++) {
    int *p = (int *)vector_at(v, i);
    assert(*p == i);
  }
}

int main(void) {
  test_vector();

  return 0;
}
