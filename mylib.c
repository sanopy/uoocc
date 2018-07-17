#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "uoocc.h"

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

void error(char *s) {
  fprintf(stderr, "Error: %s.", s);
  exit(1);
}

void error_with_token(Token *tk, char *s) {
  fprintf(stderr, "%d:%d:<%s> Error: %s.", tk->row, tk->col, tk->text, s);
  exit(1);
}