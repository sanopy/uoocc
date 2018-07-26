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
  AST_OP_EQUAL,
  AST_OP_NEQUAL,
  AST_OP_ASSIGN,
  AST_VAR,
  AST_CALL_FUNC,
  AST_DECL_FUNC,
};

typedef struct _Ast {
  int type;
  int ival;
  char *ident;
  Vector *args;
  Vector *expr;
  Map *symbol_table;
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

static Ast *make_ast_call_func(char *ident) {
  Ast *p = malloc(sizeof(Ast));
  p->type = AST_CALL_FUNC;
  p->ident = ident;
  p->left = p->right = NULL;
  p->args = vector_new();
  return p;
}

static Ast *make_ast_decl_func(char *ident) {
  Ast *p = malloc(sizeof(Ast));
  p->type = AST_DECL_FUNC;
  p->ident = ident;
  p->left = p->right = NULL;
  p->args = vector_new();
  p->expr = vector_new();
  symbol_table = p->symbol_table = map_new();
  return p;
}

static Ast *expr(void);

// <call_function> = <ident> '(' [ <expr> { ',' <expr> } ] ')'
static Ast *call_function(void) {
  Ast *p = make_ast_call_func(current_token()->text);
  expect_token(next_token(), TK_LPAR);

  if (second_token()->type == TK_RPAR) {
    next_token();
    next_token();
    return p;
  }

  do {
    next_token();
    vector_push_back(p->args, (void *)expr());
  } while (current_token()->type == TK_COMMA);

  expect_token(current_token(), TK_RPAR);

  next_token();
  return p;
}

// <primary_expr> = <ident> | <number> | '(' <expr> ')' | <call_function>
static Ast *primary_expr(void) {
  int type = current_token()->type;
  Ast *ret;
  if (type == TK_LPAR) {
    next_token();
    ret = expr();
    expect_token(current_token(), TK_RPAR);
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
  <multiplicative_expr> = <primary_expr> <multiplicative_expr_tail>
  <multiplicative_expr_tail> = ε | '*' <primary_expr> <multiplicative_expr_tail>
    | '/' <primary_expr> <multiplicative_expr_tail>
*/
static Ast *multiplicative_expr_tail(Ast *left) {
  int type = current_token()->type;
  if (type == TK_MUL || type == TK_DIV) {
    next_token();
    Ast *right = primary_expr();
    int ast_op = type == TK_MUL ? AST_OP_MUL : AST_OP_DIV;
    Ast *p = make_ast_op(ast_op, left, right);
    return multiplicative_expr_tail(p);
  } else {
    return left;
  }
}

static Ast *multiplicative_expr(Ast *prim) {
  Ast *p = prim == NULL ? primary_expr() : prim;
  return multiplicative_expr_tail(p);
}

/*
  <additive_expr> = <multiplicative_expr> <additive_expr_tail>
  <additive_expr_tail> = ε | '+' <multiplicative_expr> <additive_expr_tail> |
    '-' <multiplicative_expr> <additive_expr_tail>
*/
static Ast *additive_expr_tail(Ast *left) {
  int type = current_token()->type;
  if (type == TK_PLUS || type == TK_MINUS) {
    next_token();
    Ast *right = multiplicative_expr(NULL);
    int ast_op = type == TK_PLUS ? AST_OP_ADD : AST_OP_SUB;
    Ast *p = make_ast_op(ast_op, left, right);
    return additive_expr_tail(p);
  } else {
    return left;
  }
}

static Ast *additive_expr(Ast *prim) {
  Ast *p = multiplicative_expr(prim);
  return additive_expr_tail(p);
}

/*
  <equality_expr> = <additive_expr> <equality_expr_tail>
  <equality_expr_tail> = ε | '==' <additive_expr> <equality_expr_tail> |
    '!=' <additive_expr> <equality_expr_tail>
*/
static Ast *equality_expr_tail(Ast *left) {
  int type = current_token()->type;
  if (type == TK_EQUAL || type == TK_NEQUAL) {
    next_token();
    Ast *right = additive_expr(NULL);
    int ast_op = type == TK_EQUAL ? AST_OP_EQUAL : AST_OP_NEQUAL;
    Ast *p = make_ast_op(ast_op, left, right);
    return equality_expr_tail(p);
  } else {
    return left;
  }
}

static Ast *equality_expr(Ast *prim) {
  Ast *p = additive_expr(prim);
  return equality_expr_tail(p);
}

// <expr> = <primary_expr> '=' <expr> | <equality_expr>
static Ast *expr(void) {
  Ast *p = primary_expr();
  if (current_token()->type == TK_ASSIGN) {
    if (p->type != AST_VAR)
      error_with_token(current_token(), "cannot assign rvalue to lvalue");
    next_token();
    return make_ast_op(AST_OP_ASSIGN, p, expr());
  } else {
    return equality_expr(p);
  }
}

// <decl_function> = <ident> '(' [ <ident> { ',' <ident> } ] ')' '{'
//   { <expr> ';' } '}'
static Ast *decl_function(void) {
  expect_token(current_token(), TK_IDENT);

  Ast *p = make_ast_decl_func(current_token()->text);
  expect_token(next_token(), TK_LPAR);

  if (second_token()->type != TK_RPAR) {
    do {
      expect_token(next_token(), TK_IDENT);
      MapEntry *e = allocate_MapEntry(current_token()->text,
                                      allocate_integer(symbol_table->size + 1));
      map_put(symbol_table, e);
      vector_push_back(p->args, (void *)make_ast_var(current_token()->text));
    } while (next_token()->type == TK_COMMA);
    expect_token(current_token(), TK_RPAR);
  } else
    next_token();

  if (p->args->size > 6)
    error("too many arguments");

  expect_token(next_token(), TK_LCUR);
  while (next_token()->type != TK_RCUR) {
    vector_push_back(p->expr, (void *)expr());
    expect_token(current_token(), TK_SEMI);
  }

  next_token();
  return p;
}

// <program> = { <decl_function> }
static Vector *program(void) {
  Vector *v = vector_new();

  while (current_token()->type != TK_EOF) {
    Ast *p = decl_function();
    vector_push_back(v, (void *)p);
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
    case AST_OP_EQUAL:
    case AST_OP_NEQUAL:
      codegen(p->left);
      codegen(p->right);
      printf("\tpopq %%rdx\n");
      printf("\tpopq %%rax\n");
      printf("\tcmpl %%edx, %%eax\n");
      printf("\t%s %%al\n", p->type == AST_OP_EQUAL ? "sete" : "setne");
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
      for (int i = p->args->size - 1; i >= 0; i--)
        codegen((Ast *)vector_at(p->args, i));

      for (int i = 1; i <= (p->args->size > 6 ? 6 : p->args->size); i++) {
        char *reg[] = {"", "rdi", "rsi", "rdx", "rcx", "r8", "r9"};
        printf("\tpopq %%%s\n", reg[i]);
      }
      printf("\tcall %s\n", p->ident);
      printf("\tpushq %%rax\n");
      break;
    case AST_DECL_FUNC:
      symbol_table = p->symbol_table;
      printf("%s:\n", p->ident);
      printf("\tpushq %%rbp\n");
      printf("\tmovq %%rsp, %%rbp\n");
      if (symbol_table->size > 0 && (symbol_table->size * 4) % 16 == 0)
        printf("\tsub $%d, %%rsp\n", symbol_table->size * 4);
      else if (p->symbol_table->size > 0)
        printf("\tsub $%d, %%rsp\n",
               (symbol_table->size * 4) + (16 - (symbol_table->size * 4) % 16));

      for (int i = 0; i < (p->args->size > 6 ? 6 : p->args->size); i++) {
        char *s = ((Ast *)vector_at(p->args, i))->ident;
        char *reg[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
        printf("\tmovl %%%s, %d(%%rbp)\n", reg[i],
               *(int *)(map_get(symbol_table, s)->val) * -4);
      }

      for (int i = 0; i < p->expr->size; i++)
        codegen((Ast *)vector_at(p->expr, i));
      printf("\tpopq %%rax\n");
      printf("\tmovq %%rbp, %%rsp\n");
      printf("\tpopq %%rbp\n");
      printf("\tret\n");
      break;
  }
}

int main(void) {
  init_token_queue(stdin);
  Vector *v = program();

  printf("\t.global main\n");
  for (int i = 0; i < v->size; i++)
    codegen((Ast *)vector_at(v, i));

  return 0;
}
