#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

enum {
  AST_INT,
  AST_OP_ADD,
  AST_OP_SUB,
  AST_OP_MUL,
  AST_OP_DIV,
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

Ast *expr(void);

// <factor> = <number> | '(' <expr> ')'
Ast *factor(void) {
  skip();
  char c = getc(stdin);

  Ast *ret;
  if (c == '(') {
    ret = expr();
    skip();
    c = getc(stdin);
    if (c != ')')
      error("')' was expected");
  } else {
    ungetc(c, stdin);
    ret = read_number();
  }

  return ret;
}

/*
  <term> = <factor> <term_tail>
  <term_tail> = ε | '*' <factor> <term_tail> | '/' <factor> <term_tail>
*/
Ast *term_tail(Ast *left) {
  skip();
  char c = getc(stdin);

  if (c == '*' || c == '/') {
    Ast *right = factor();
    int ast_op = c == '*' ? AST_OP_MUL : AST_OP_DIV;
    Ast *p = make_ast_op(ast_op, left, right);
    return term_tail(p);
  } else {
    ungetc(c, stdin);
    return left;
  }
}

Ast *term(void) {
  Ast *val = factor();
  return term_tail(val);
}

/*
  <expr> = <term> <expr_tail>
  <expr_tail> = ε | '+' <term> <expr_tail> | '-' <term> <expr_tail>
*/
Ast *expr_tail(Ast *left) {
  skip();
  char c = getc(stdin);

  if (c == '+' || c == '-') {
    Ast *right = term();
    int ast_op = c == '+' ? AST_OP_ADD : AST_OP_SUB;
    Ast *p = make_ast_op(ast_op, left, right);
    return expr_tail(p);
  } else {
    ungetc(c, stdin);
    return left;
  }
}

Ast *expr(void) {
  Ast *val = term();
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
  } else if (p->type == AST_OP_MUL || p->type == AST_OP_DIV) {
    printf("(%c ", p->type == AST_OP_MUL ? '*' : '/');
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
    break;
  case AST_OP_MUL:
  case AST_OP_DIV:
    codegen(p->left);
    codegen(p->right);
    printf("\tpopq %%rbx\n");
    printf("\tpopq %%rax\n");
    if (p->type == AST_OP_MUL)
      printf("\tmul %%rbx\n");
    else {
      printf("\txor %%rdx, %%rdx\n");
      printf("\tdiv %%rbx\n");
    }
    printf("\tpushq %%rax\n");
    break;
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
