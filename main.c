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
  AST_OP_POST_INC,
  AST_OP_POST_DEC,
  AST_OP_PRE_INC,
  AST_OP_PRE_DEC,
  AST_OP_LT,
  AST_OP_LE,
  AST_OP_GT,
  AST_OP_GE,
  AST_OP_EQUAL,
  AST_OP_NEQUAL,
  AST_OP_ASSIGN,
  AST_VAR,
  AST_CALL_FUNC,
  AST_DECL_FUNC,
  AST_COMPOUND_STATEMENT,
  AST_IF_STATEMENT
};

typedef struct _Ast {
  int type;
  int ival;
  char *ident;
  Vector *args;
  Vector *statement;
  Map *symbol_table;
  struct _Ast *left;
  struct _Ast *right;
  struct _Ast *cond;
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
  symbol_table = p->symbol_table = map_new();
  return p;
}

static Ast *make_ast_compound_statement(void) {
  Ast *p = malloc(sizeof(Ast));
  p->type = AST_COMPOUND_STATEMENT;
  p->left = p->right = NULL;
  p->statement = vector_new();
  return p;
}

static Ast *make_ast_if_statement(void) {
  Ast *p = malloc(sizeof(Ast));
  p->type = AST_IF_STATEMENT;
  p->left = p->right = NULL;
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
  <postfix_expr> = <primary_expr> <postfix_expr_tail>
  <postfix_expr_tail> = ε | '++' <postfix_expr_tail> | '--' <postfix_expr_tail>
*/
static Ast *postfix_expr_tail(Ast *left) {
  int type = current_token()->type;
  if (type == TK_INC || type == TK_DEC) {
    if (left == NULL || left->type != AST_VAR)
      error_with_token(current_token(), "expression is not assignable");
    next_token();
    Ast *right = postfix_expr_tail(NULL);
    int ast_op = type == TK_INC ? AST_OP_POST_INC : AST_OP_POST_DEC;
    Ast *p = make_ast_op(ast_op, left, right);
    return postfix_expr_tail(p);
  } else {
    return left;
  }
}

static Ast *postfix_expr(void) {
  Ast *p = primary_expr();
  return postfix_expr_tail(p);
}

/*
  <unary_expr> = <postfix_expr> | '++' <unary_expr> | '--' <unary_expr>
*/
static Ast *unary_expr(void) {
  Token *tk = current_token();
  int type = tk->type;
  if (type == TK_INC || type == TK_DEC) {
    next_token();
    Ast *left = unary_expr();
    if (left == NULL || left->type != AST_VAR)
      error_with_token(tk, "expression is not assignable");
    int ast_op = type == TK_INC ? AST_OP_PRE_INC : AST_OP_PRE_DEC;
    Ast *p = make_ast_op(ast_op, left, NULL);
    return postfix_expr_tail(p);
  } else {
    return postfix_expr();
  }
}

/*
  <multiplicative_expr> = <unary_expr> <multiplicative_expr_tail>
  <multiplicative_expr_tail> = ε | '*' <unary_expr> <multiplicative_expr_tail>
    | '/' <unary_expr> <multiplicative_expr_tail>
*/
static Ast *multiplicative_expr_tail(Ast *left) {
  int type = current_token()->type;
  if (type == TK_MUL || type == TK_DIV) {
    next_token();
    Ast *right = unary_expr();
    int ast_op = type == TK_MUL ? AST_OP_MUL : AST_OP_DIV;
    Ast *p = make_ast_op(ast_op, left, right);
    return multiplicative_expr_tail(p);
  } else {
    return left;
  }
}

static Ast *multiplicative_expr(Ast *unary) {
  Ast *p = unary == NULL ? unary_expr() : unary;
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

static Ast *additive_expr(Ast *unary) {
  Ast *p = multiplicative_expr(unary);
  return additive_expr_tail(p);
}

/*
  <relational_expr> = <additive_expr> <relational_expr_tail>
  <relational_expr_tail> = ε | '<' <additive_expr> <relational_expr_tail> |
    '<=' <additive_expr> <relational_expr_tail> |
    '>'  <additive_expr> <relational_expr_tail> |
    '>=' <additive_expr> <relational_expr_tail>
*/
static Ast *relational_expr_tail(Ast *left) {
  int type = current_token()->type;
  if (type == TK_LT || type == TK_LE || type == TK_GT || type == TK_GE) {
    next_token();
    Ast *right = additive_expr(NULL);
    int ast_op;
    if (type == TK_LT)
      ast_op = AST_OP_LT;
    else if (type == TK_LE)
      ast_op = AST_OP_LE;
    else if (type == TK_GT)
      ast_op = AST_OP_GT;
    else
      ast_op = AST_OP_GE;
    Ast *p = make_ast_op(ast_op, left, right);
    return relational_expr_tail(p);
  } else {
    return left;
  }
}

static Ast *relational_expr(Ast *unary) {
  Ast *p = additive_expr(unary);
  return relational_expr_tail(p);
}

/*
  <equality_expr> = <relational_expr> <equality_expr_tail>
  <equality_expr_tail> = ε | '==' <relational_expr> <equality_expr_tail> |
    '!=' <relational_expr> <equality_expr_tail>
*/
static Ast *equality_expr_tail(Ast *left) {
  int type = current_token()->type;
  if (type == TK_EQUAL || type == TK_NEQUAL) {
    next_token();
    Ast *right = relational_expr(NULL);
    int ast_op = type == TK_EQUAL ? AST_OP_EQUAL : AST_OP_NEQUAL;
    Ast *p = make_ast_op(ast_op, left, right);
    return equality_expr_tail(p);
  } else {
    return left;
  }
}

static Ast *equality_expr(Ast *unary) {
  Ast *p = relational_expr(unary);
  return equality_expr_tail(p);
}

// <expr> = <unary_expr> '=' <expr> | <equality_expr>
static Ast *expr(void) {
  Ast *p = unary_expr();
  if (current_token()->type == TK_ASSIGN) {
    if (p->type != AST_VAR)
      error_with_token(current_token(), "expression is not assignable");
    next_token();
    return make_ast_op(AST_OP_ASSIGN, p, expr());
  } else {
    return equality_expr(p);
  }
}

static Ast *statement(void);

// <selection_statement> = 'if' '(' <expression> ')' <statement>
//   [ 'else' <statement> ]
static Ast *selection_statement(void) {
  // current token is 'if' when enter this function.
  Ast *p = make_ast_if_statement();

  expect_token(next_token(), TK_LPAR);
  next_token();
  p->cond = expr();
  expect_token(current_token(), TK_RPAR);
  next_token();
  p->left = statement();

  if (current_token()->type == TK_ELSE) {
    next_token();
    p->right = statement();
  }

  return p;
}

// <compound_statement> = '{' { <statement> } '}'
static Ast *compound_statement(void) {
  // current token is '{' when enter this function.
  Ast *p = make_ast_compound_statement();

  next_token();
  while (current_token()->type != TK_RCUR)
    vector_push_back(p->statement, (void *)statement());
  next_token();

  return p;
}

static Ast *expr_statement(void) {
  Ast *p = expr();
  expect_token(current_token(), TK_SEMI);
  next_token();
  return p;
}

// <statement> = <selection_statement> | <compound_statement> | <expr_statement>
static Ast *statement(void) {
  if (current_token()->type == TK_IF)
    return selection_statement();
  else if (current_token()->type == TK_LCUR)
    return compound_statement();
  else
    return expr_statement();
}

// <decl_function> = <ident> '(' [ <ident> { ',' <ident> } ] ')'
//   <compound_statement>
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
  p->left = compound_statement();

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
    case AST_OP_POST_INC:
    case AST_OP_POST_DEC:
      printf("\tpushq %d(%%rbp)\n",
             *(int *)(map_get(symbol_table, p->left->ident)->val) * -4);
      printf("\t%s %d(%%rbp)\n", p->type == AST_OP_POST_INC ? "incl" : "decl",
             *(int *)(map_get(symbol_table, p->left->ident)->val) * -4);
      break;
    case AST_OP_PRE_INC:
    case AST_OP_PRE_DEC:
      printf("\t%s %d(%%rbp)\n", p->type == AST_OP_PRE_INC ? "incl" : "decl",
             *(int *)(map_get(symbol_table, p->left->ident)->val) * -4);
      printf("\tpushq %d(%%rbp)\n",
             *(int *)(map_get(symbol_table, p->left->ident)->val) * -4);
      break;
    case AST_OP_LT:
    case AST_OP_LE:
    case AST_OP_GT:
    case AST_OP_GE:
    case AST_OP_EQUAL:
    case AST_OP_NEQUAL:
      codegen(p->left);
      codegen(p->right);
      printf("\tpopq %%rdx\n");
      printf("\tpopq %%rax\n");
      printf("\tcmpl %%edx, %%eax\n");
      char *s;
      if (p->type == AST_OP_LT)
        s = allocate_string("setl");
      else if (p->type == AST_OP_LE)
        s = allocate_string("setle");
      else if (p->type == AST_OP_GT)
        s = allocate_string("setg");
      else if (p->type == AST_OP_GE)
        s = allocate_string("setge");
      else if (p->type == AST_OP_EQUAL)
        s = allocate_string("sete");
      else
        s = allocate_string("setne");
      printf("\t%s %%al\n", s);
      printf("\tmovzbl %%al, %%eax\n");
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

      codegen(p->left);

      printf("\tpopq %%rax\n");
      printf("\tmovq %%rbp, %%rsp\n");
      printf("\tpopq %%rbp\n");
      printf("\tret\n");
      break;
    case AST_COMPOUND_STATEMENT:
      for (int i = 0; i < p->statement->size; i++)
        codegen((Ast *)vector_at(p->statement, i));
      break;
    case AST_IF_STATEMENT:
      codegen(p->cond);
      printf("\tpopq %%rax\n");
      printf("\ttest %%rax, %%rax\n");
      int seq1 = get_sequence_num();
      printf("\tjz .L%d\n", seq1);
      codegen(p->left);
      if (p->right != NULL) {
        int seq2 = get_sequence_num();
        printf("\tjmp .L%d\n", seq2);
        printf(".L%d:\n", seq1);
        codegen(p->right);
        printf(".L%d:\n", seq2);
      } else
        printf(".L%d:\n", seq1);
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
