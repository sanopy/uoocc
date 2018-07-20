#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "uoocc.h"

enum {
  AST_INT,
  AST_OP_ADD,
  AST_OP_SUB,
  AST_OP_MUL,
  AST_OP_DIV,
  AST_OP_ASSIGN,
  AST_VAR,
  AST_CALL_FUNC,
};

typedef struct _Ast {
  int type;
  int ival;
  char *ident;
  struct _Ast *left;
  struct _Ast *right;
} Ast;

TokenQueue token_queue;
Map *symbol_table;

static Ast *make_ast_op(int type, Ast *left, Ast *right) {
  Ast *p = malloc(sizeof(Ast));
  p->type = type;
  p->left = left;
  p->right = right;
  return p;
}

static Ast *make_ast_int(int val) {
  Ast *p = malloc(sizeof(Ast));
  p->type = AST_INT;
  p->ival = val;
  p->left = p->right = NULL;
  return p;
}

static Ast *make_ast_var(char *ident) {
  Ast *p = malloc(sizeof(Ast));
  p->type = AST_VAR;
  p->ident = ident;
  p->left = p->right = NULL;
  return p;
}

static Ast *make_ast_func(char *ident) {
  Ast *p = malloc(sizeof(Ast));
  p->type = AST_CALL_FUNC;
  p->ident = ident;
  p->left = p->right = NULL;
  return p;
}

// <call_function> = <ident> '(' ')'
static Ast *call_function(void) {
  Ast *p = make_ast_func(current_token()->text);
  if (next_token()->type != TK_LPAR)
    error_with_token(current_token(), "'(' was expected");
  if (next_token()->type != TK_RPAR)
    error_with_token(current_token(), "')' was expected");
  next_token();
  return p;
}

static Ast *expr(void);

// <factor> = <variable> | <number> | '(' <expr> ')' | <call_function>
static Ast *factor(void) {
  int type = current_token()->type;
  Ast *ret;
  if (type == TK_LPAR) {
    next_token();
    ret = expr();
    if (current_token()->type != TK_RPAR)
      error_with_token(current_token(), "')' was expected");
    next_token();
  } else if (type == TK_NUM) {
    ret = make_ast_int(current_token()->number);
    next_token();
  } else if (current_token()->type == TK_IDENT &&
             second_token()->type == TK_LPAR) {
    ret = call_function();
  } else {
    if (map_get(symbol_table, current_token()->text) == NULL) {
      MapEntry *e = allocate_MapEntry(current_token()->text,
                                      allocate_integer(symbol_table->size + 1));
      map_put(symbol_table, e);
    }
    ret = make_ast_var(current_token()->text);
    next_token();
  }

  return ret;
}

/*
  <term> = <factor> <term_tail>
  <term_tail> = ε | '*' <factor> <term_tail> | '/' <factor> <term_tail>
*/
static Ast *term_tail(Ast *left) {
  int type = current_token()->type;
  if (type == TK_MUL || type == TK_DIV) {
    next_token();
    Ast *right = factor();
    int ast_op = type == TK_MUL ? AST_OP_MUL : AST_OP_DIV;
    Ast *p = make_ast_op(ast_op, left, right);
    return term_tail(p);
  } else {
    return left;
  }
}

static Ast *term(void) {
  Ast *val = factor();
  return term_tail(val);
}

/*
  <expr> = <variable> '=' <expr> | <term> <expr_tail>
  <expr_tail> = ε | '+' <term> <expr_tail> | '-' <term> <expr_tail>
*/
static Ast *expr_tail(Ast *left) {
  int type = current_token()->type;
  if (type == TK_PLUS || type == TK_MINUS) {
    next_token();
    Ast *right = term();
    int ast_op = type == TK_PLUS ? AST_OP_ADD : AST_OP_SUB;
    Ast *p = make_ast_op(ast_op, left, right);
    return expr_tail(p);
  } else {
    return left;
  }
}

static Ast *expr(void) {
  Ast *p = term();
  if (p->type == AST_VAR && current_token()->type == TK_ASSIGN) {
    next_token();
    return make_ast_op(AST_OP_ASSIGN, p, expr());
  } else {
    return expr_tail(p);
  }
}

// <program> = { <expr> ';' }
static Vector *program(void) {
  Vector *v = vector_new();

  while (current_token()->type != TK_EOF) {
    Ast *p = expr();
    vector_push_back(v, (void *)p);
    if (current_token()->type != TK_SEMI)
      error_with_token(current_token(), "';' was expected");
    else
      next_token();
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
  } else if (p->type == AST_VAR) {
    printf("%s", p->ident);
  }
}

static void codegen(Ast *p) {
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
      printf("\t%s %%edx, %%eax\n", p->type == AST_OP_ADD ? "add" : "sub");
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
      printf("\tmovl %%eax, %d(%%rbp)\n",
             *(int *)(map_get(symbol_table, p->left->ident)->val) * -4);
      printf("\tpushq %%rax\n");
      break;
    case AST_VAR:
      printf("\tpushq %d(%%rbp)\n",
             *(int *)(map_get(symbol_table, p->ident)->val) * -4);
      break;
    case AST_CALL_FUNC:
      printf("\tcall %s\n", p->ident);
      printf("\tpushq %%rax\n");
      break;
  }
}

int main(void) {
  init_token_queue(stdin);
  symbol_table = map_new();
  Vector *v = program();

  printf("\t.global main\n");
  printf("main:\n");
  printf("\tpushq %%rbp\n");
  printf("\tmovq %%rsp, %%rbp\n");
  printf("\tsub $%d, %%rsp\n", symbol_table->size * 4);

  for (int i = 0; i < v->size; i++)
    codegen((Ast *)vector_at(v, i));

  printf("\tpopq %%rax\n");

  printf("\tmovq %%rbp, %%rsp\n");
  printf("\tpopq %%rbp\n");
  printf("\tret\n");

  return 0;
}
