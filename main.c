#include <stdio.h>

int main(void) {
  int val;
  scanf("%d", &val);

  printf("\t.global main\n");
  printf("main:\n");
  printf("\tmovl $%d, %%eax\n", val);
  printf("\tret\n");

  return 0;
}
