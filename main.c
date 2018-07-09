#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

void skip(void) {
  char c;
  do {
    c = getc(stdin);
  } while(isspace(c));
  ungetc(c, stdin);
}

int read_number(void) {
  char c;
  int ret = 0;

  skip();

  while ((c = getc(stdin)) != EOF) {
    if (!isdigit(c)) {
      ungetc(c, stdin);
      break;
    }
    ret = ret * 10 + c - '0';
  }

  return ret;
}

void error(char *s) {
  fprintf(stderr, "Error: %s.", s);
  exit(1);
}

/*
  expression ::= number | number [ expression_add | expression_sub ]
  expression_add ::= '+' expression
  expression_sub ::= '-' expression
*/
void expression(char operation) {
  int val;
  char c;

  if (operation == 0) {
    val = read_number();
    printf("\tmovq $%d, %%rax\n", val);
    skip(); c = getc(stdin);
  } else {
    printf("\tpopq %%rax\n");
    c = operation;
  }

  switch (c) {
  case '+':
  case '-':
    val = read_number();
    char *op = c == '+' ? "add" : "sub";
    printf("\t%s $%d, %%rax\n", op, val);
    printf("\tpushq %%rax\n");
    skip(); c = getc(stdin);
    if (c == '+')
      expression('+');
    else if (c == '-')
      expression('-');
    else if (c == '\n' || c == EOF)
      return;
    else
      error("unexpected character was inputted");
    break;
  case '\n':
  case EOF:
    printf("\tpushq %%rax\n");
    return;
  default:
    error("unexpected character was inputted");
  }
}

int main(void) {
  printf("\t.global main\n");
  printf("main:\n");
  expression(0);
  printf("\tpopq %%rax\n");
  printf("\tret\n");

  return 0;
}
