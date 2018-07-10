#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

enum {
  AST_INT,
  AST_OP_ADD,
  AST_OP_SUB,
};

typedef struct _Ast {
  int type;
  int ival;
  struct _Ast *left;
  struct _Ast *right;
} Ast;

Ast *make_ast_op(int type, Ast *left, Ast *right) {
  Ast *p = malloc(sizeof(Ast));
  p->type = type;
  p->left = left;
  p->right = right;
  return p;
}

Ast *make_ast_int(int val) {
  Ast *p = malloc(sizeof(Ast));
  p->type = AST_INT;
  p->ival = val;
  p->left = p->right = NULL;
  return p;
}

void skip(void) {
  char c;
  do {
    c = getc(stdin);
  } while(isspace(c));
  ungetc(c, stdin);
}

void error(char *s) {
  fprintf(stderr, "Error: %s.", s);
  exit(1);
}

Ast *read_number(void) {
  char c;
  int num = 0;

  skip();

  c = getc(stdin);
  if (!isdigit(c))
    error("number was expected");
  else
    ungetc(c, stdin);

  while ((c = getc(stdin)) != EOF) {
    if (!isdigit(c)) {
      ungetc(c, stdin);
      break;
    }
    num = num * 10 + c - '0';
  }

  return make_ast_int(num);
}

/*
  <expr> = <number> <expr_tail>
  <expr_tail> = ε | '+' <number> <expr_tail> | '-' <number> <expr_tail>
*/
Ast *expr_tail(Ast *left) {
  skip();
  char c = getc(stdin);

  if (c == '+' || c == '-') {
    Ast *right = read_number();
    int ast_op = c == '+' ? AST_OP_ADD : AST_OP_SUB;
    Ast *p = make_ast_op(ast_op, left, right);
    return expr_tail(p);
  } else if (c == '\n' || c == EOF)
    return left;
  else
    error("unexpected character was inputted");

  return NULL; // NOTE: To avoid warning. Actually, cannot reach here.
}

Ast *expr(void) {
  Ast *val = read_number();
  return expr_tail(val);
}

void debug_print(Ast *p) {
  if (p == NULL)
    return;
  else if (p->type == AST_INT)
    printf("%d", p->ival);
  else if (p->type == AST_OP_ADD || p->type == AST_OP_SUB) {
    printf("(%c ", p->type == AST_OP_ADD ? '+' : '-');
    debug_print(p->left);
    printf(" ");
    debug_print(p->right);
    printf(")");
  }
}

void codegen(Ast *p) {
  if (p == NULL)
    return;

  switch (p->type) {
  case AST_INT:
    printf("\tpushq $%d\n", p->ival);
    break;
  case AST_OP_ADD:
  case AST_OP_SUB:
    codegen(p->left);
    codegen(p->right);
    printf("\tpopq %%rbx\n");
    printf("\tpopq %%rax\n");
    printf("\t%s %%rbx, %%rax\n", p->type == AST_OP_ADD ? "add" : "sub");
    printf("\tpushq %%rax\n");
  }
}

int main(void) {
  Ast *root = expr();

  printf("\t.global main\n");
  printf("main:\n");
  codegen(root);
  printf("\tpopq %%rax\n");
  printf("\tret\n");

  return 0;
}
