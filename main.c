#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "uoocc.h"

#define MAX_IDENT_LENGTH 256

enum {
  AST_INT,
  AST_OP_ADD,
  AST_OP_SUB,
  AST_OP_MUL,
  AST_OP_DIV,
  AST_OP_ASSIGN,
  AST_IDENT,
};

typedef struct _Ast {
  int type;
  int ival;
  char *ident;
  struct _Ast *left;
  struct _Ast *right;
} Ast;

Map *symbol_table;

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

Ast *make_ast_ident(char *ident) {
  Ast *p = malloc(sizeof(Ast));
  p->type = AST_IDENT;
  p->ident = ident;
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

// [0-9]+
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

// [a-zA-Z]+
Ast *read_ident(void) {
  char ident[MAX_IDENT_LENGTH];
  int idx = 0;

  skip();

  char c = getc(stdin);
  while (isalpha(c)) {
    ident[idx++] = c;
    c = getc(stdin);
  }

  if (idx == 0)
    error("ident was expected");

  ident[idx] = '\0';
  ungetc(c, stdin);

  if (map_get(symbol_table, ident) == NULL) {
    MapEntry *e = allocate_MapEntry(allocate_string(ident), allocate_integer(symbol_table->size + 1));
    map_put(symbol_table, e);
  }

  return make_ast_ident(allocate_string(ident));
}

Ast *expr(void);

// <factor> = <variable> | <number> | '(' <expr> ')'
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
  } else if (isdigit(c)) {
    ungetc(c, stdin);
    ret = read_number();
  } else {
    ungetc(c, stdin);
    ret = read_ident();
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
  <expr> = <variable> '=' <expr> | <term> <expr_tail>
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
  Ast *p = term();
  skip(); char c = getc(stdin);
  if (p->type == AST_IDENT && c == '=')
    return make_ast_op(AST_OP_ASSIGN, p, expr());
  else {
    ungetc(c, stdin);
    return expr_tail(p);
  }
}

// <program> = { <expr> ';' }
Vector *program(void) {
  Vector *v = vector_new();
  skip(); char c = getc(stdin);

  while (c != EOF) {
    ungetc(c, stdin);
    Ast *p = expr();
    vector_push_back(v, (void *)p);
    skip(); c = getc(stdin);
    if (c != ';')
      error("';' was wxpected");
    skip(); c = getc(stdin);
  }

  return v;
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
  } else if (p->type == AST_OP_ASSIGN) {
    printf("(= ");
    debug_print(p->left);
    printf(" ");
    debug_print(p->right);
    printf(")");
  } else if (p->type == AST_IDENT) {
    printf("%s", p->ident);
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
    printf("\tpopq %%rdx\n");
    printf("\tpopq %%rax\n");
    printf("\t%s %%rdx, %%rax\n", p->type == AST_OP_ADD ? "add" : "sub");
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
  case AST_OP_ASSIGN:
    codegen(p->right);
    printf("\tpopq %%rax\n");
    printf("\tmovq %%rax, %d(%%rbp)\n", *(int *)(map_get(symbol_table, p->left->ident)->val) * -8);
    break;
  case AST_IDENT:
    printf("\tpushq %d(%%rbp)\n", *(int *)(map_get(symbol_table, p->ident)->val) * -8);
    break;
  }
}

int main(void) {
  symbol_table = map_new();
  Vector *v = program();

  printf("\t.global main\n");
  printf("main:\n");
  printf("\tpushq %%rbp\n");
  printf("\tmovq %%rsp, %%rbp\n");
  printf("\tsub $%d, %%rsp\n", symbol_table->size * 8);

  for (int i = 0; i < v->size; i++)
    codegen((Ast *)vector_at(v, i));

  printf("\tpopq %%rax\n");

  printf("\tmovq %%rbp, %%rsp\n");
  printf("\tpopq %%rbp\n");
  printf("\tret\n");

  return 0;
}
