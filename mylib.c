#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uoocc.h"

static char *allocate_concat_string(char *s1, char *s2) {
  char *p = (char *)malloc(sizeof(char) * (strlen(s1) + strlen(s2) + 1));
  strcpy(p, s1);
  strcat(p, s2);
  return p;
}

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
  fprintf(stderr, "Error: %s.\n", s);
  exit(1);
}

void error_with_token(Token *tk, char *s) {
  fprintf(stderr, "%d:%d:<%s> Error: %s.\n", tk->row, tk->col, tk->text, s);
  exit(1);
}

void expect_token(Token *tk, int expect) {
  char *token[] = {"EOF", "number", "ident", "'+'", "'-'", "'*'", "'/'", "'('", "')'", "'='", "';'", "','", "'{'", "'}'"};
  if (tk->type != expect)
    error_with_token(tk, allocate_concat_string(token[expect], " was expected"));
}
