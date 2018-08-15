#include "uoocc.h"

int main(void) {
  init_token_queue(stdin);
  symbol_table = map_new(NULL);
  Vector *v = program();

  for (int i = 0; i < v->size; i++)
    semantic_analysis(vector_at(v, i));

  printf("\t.global main\n");
  for (int i = 0; i < v->size; i++)
    codegen(vector_at(v, i));

  return 0;
}
