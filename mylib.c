#include <string.h>
#include <stdlib.h>

char *allocate_string(char *s) {
  char *p = (char *)malloc(sizeof(char) * (strlen(s) + 1));
  strcpy(p, s);
  return p;
}

int *allocate_integer(int n) {
  int *p = (int *)malloc(sizeof(int));
  *p = n;
  return p;
}
