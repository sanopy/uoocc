#include <stdio.h>
#include <stdlib.h>

void print_hello(void) { puts("hello"); }

int return_seven(void) { return 7; }

void print_n(int n) { printf("%d\n", n); }

void print_args(int _1, int _2, int _3, int _4, int _5, int _6, int _7,
                int _8) {
  printf("%d,%d,%d,%d,%d,%d,%d,%d\n", _1, _2, _3, _4, _5, _6, _7, _8);
}

int add_three_args(int x, int y, int z) { return x + y + z; }

int add_eight_args(int _1, int _2, int _3, int _4, int _5, int _6, int _7,
                   int _8) {
  return _1 + _2 + _3 + _4 + _5 + _6 + _7 + _8;
}

void alloc4(int **p, int w, int x, int y, int z) {
  *p = (int *)malloc(sizeof(int) * 4);
  (*p)[0] = w;
  (*p)[1] = x;
  (*p)[2] = y;
  (*p)[3] = z;
}
