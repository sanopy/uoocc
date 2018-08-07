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
  AST_OP_REF,
  AST_OP_DEREF,
  AST_OP_LT,
  AST_OP_LE,
  AST_OP_GT,
  AST_OP_GE,
  AST_OP_EQUAL,
  AST_OP_NEQUAL,
  AST_OP_ASSIGN,
  AST_VAR,
  AST_DECLARATION,
  AST_CALL_FUNC,
  AST_DECL_FUNC,
  AST_COMPOUND_STATEMENT,
  AST_IF_STATEMENT,
  AST_WHILE_STATEMENT,
  AST_FOR_STATEMENT,
};

enum {
  TYPE_INT,
  TYPE_PTR,
};

typedef struct _CType {
  int type;
  struct _CType *ptrof;
} CType;

typedef struct {
  CType *ctype;
  int offset;
} SymbolTableEntry;

typedef struct _Ast {
  int type;
  CType *ctype;
  int ival;
  char *ident;
  Vector *args;
  Vector *statements;
  Map *symbol_table;
  int offset_from_bp;
  Token *token;
  struct _Ast *left;
  struct _Ast *right;
  struct _Ast *cond;
  struct _Ast *init;
  struct _Ast *step;
  struct _Ast *statement;
} Ast;

TokenQueue token_queue;
Map *symbol_table;

static CType *make_ctype(int type, CType *ptrof) {
  CType *p = (CType *)malloc(sizeof(CType));
  p->type = type;
  p->ptrof = ptrof;
  return p;
}

static Ast *make_ast_op(int type, Ast *left, Ast *right, Token *token) {
  Ast *p = malloc(sizeof(Ast));
  p->type = type;
  p->left = left;
  p->right = right;
  p->token = token;
  return p;
}

static Ast *make_ast_int(int val) {
  Ast *p = malloc(sizeof(Ast));
  p->type = AST_INT;
  p->ctype = make_ctype(TYPE_INT, NULL);
  p->ival = val;
  return p;
}

static Ast *make_ast_var(char *ident, Token *token) {
  Ast *p = malloc(sizeof(Ast));
  p->type = AST_VAR;
  p->ident = ident;
  p->token = token;
  return p;
}

static Ast *make_ast_declaration(CType *ctype, char *ident, Token *token) {
  Ast *p = malloc(sizeof(Ast));
  p->type = AST_DECLARATION;
  p->ctype = ctype;
  p->ident = ident;
  p->token = token;
  return p;
}

static Ast *make_ast_call_func(char *ident) {
  Ast *p = malloc(sizeof(Ast));
  p->type = AST_CALL_FUNC;
  p->ctype = make_ctype(TYPE_INT, NULL);
  p->ident = ident;
  p->args = vector_new();
  return p;
}

static Ast *make_ast_decl_func(char *ident, Token *token) {
  Ast *p = malloc(sizeof(Ast));
  p->type = AST_DECL_FUNC;
  p->ident = ident;
  p->token = token;
  p->args = vector_new();
  p->symbol_table = map_new();
  return p;
}

static Ast *make_ast_compound_statement(void) {
  Ast *p = malloc(sizeof(Ast));
  p->type = AST_COMPOUND_STATEMENT;
  p->statements = vector_new();
  return p;
}

static Ast *make_ast_statement(int type) {
  Ast *p = malloc(sizeof(Ast));
  p->type = type;
  return p;
}

static SymbolTableEntry *make_SymbolTableEntry(CType *ctype, int offset) {
  SymbolTableEntry *p = (SymbolTableEntry *)malloc(sizeof(SymbolTableEntry));
  p->ctype = ctype;
  p->offset = offset;
  return p;
}

int offset_from_bp;
static int get_offset_from_bp(int type) {
  if (type == TYPE_INT)
    offset_from_bp += 4;
  else if (type == TYPE_PTR) {
    if (offset_from_bp % 8 == 0)
      offset_from_bp += 8;
    else
      offset_from_bp += 8 - offset_from_bp % 8 + 8;
  }
  return offset_from_bp;
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
  Ast *ret = NULL;
  if (type == TK_LPAR) {
    next_token();
    ret = expr();
    expect_token(current_token(), TK_RPAR);
    next_token();
  } else if (type == TK_NUM) {
    ret = make_ast_int(current_token()->number);
    next_token();
  } else if (type == TK_IDENT && second_token()->type == TK_LPAR) {
    ret = call_function();
  } else if (type == TK_IDENT) {
    ret = make_ast_var(current_token()->text, current_token());
    next_token();
  } else
    error_with_token(current_token(), "primary-expression was expected");

  return ret;
}

/*
  <postfix_expr> = <primary_expr> <postfix_expr_tail>
  <postfix_expr_tail> = ε | '++' <postfix_expr_tail> | '--' <postfix_expr_tail>
*/
static Ast *postfix_expr_tail(Ast *left) {
  int type = current_token()->type;
  if (type == TK_INC || type == TK_DEC) {
    Token *tk = current_token();
    next_token();
    Ast *right = postfix_expr_tail(NULL);
    int ast_op = type == TK_INC ? AST_OP_POST_INC : AST_OP_POST_DEC;
    Ast *p = make_ast_op(ast_op, left, right, tk);
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
  <unary_expr> = <postfix_expr> | '++' <unary_expr> | '--' <unary_expr> |
    '&' <unary_expr> | '*' <unary_expr>
*/
static Ast *unary_expr(void) {
  Token *tk = current_token();
  int type = tk->type;
  int ast_op;

  if (type == TK_INC)
    ast_op = AST_OP_PRE_INC;
  else if (type == TK_DEC)
    ast_op = AST_OP_PRE_DEC;
  else if (type == TK_AMP)
    ast_op = AST_OP_REF;
  else if (type == TK_STAR)
    ast_op = AST_OP_DEREF;
  else
    ast_op = -1;

  if (ast_op != -1) {
    next_token();
    Ast *left = unary_expr();
    Ast *p = make_ast_op(ast_op, left, NULL, tk);
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
  Token *tk = current_token();
  int type = tk->type;
  if (type == TK_STAR || type == TK_DIV) {
    next_token();
    Ast *right = unary_expr();
    int ast_op = type == TK_STAR ? AST_OP_MUL : AST_OP_DIV;
    Ast *p = make_ast_op(ast_op, left, right, tk);
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
  Token *tk = current_token();
  int type = tk->type;
  if (type == TK_PLUS || type == TK_MINUS) {
    next_token();
    Ast *right = multiplicative_expr(NULL);
    int ast_op = type == TK_PLUS ? AST_OP_ADD : AST_OP_SUB;
    Ast *p = make_ast_op(ast_op, left, right, tk);
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
  Token *tk = current_token();
  int type = tk->type;
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
    Ast *p = make_ast_op(ast_op, left, right, tk);
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
  Token *tk = current_token();
  int type = tk->type;
  if (type == TK_EQUAL || type == TK_NEQUAL) {
    next_token();
    Ast *right = relational_expr(NULL);
    int ast_op = type == TK_EQUAL ? AST_OP_EQUAL : AST_OP_NEQUAL;
    Ast *p = make_ast_op(ast_op, left, right, tk);
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
    Token *tk = current_token();
    next_token();
    return make_ast_op(AST_OP_ASSIGN, p, expr(), tk);
  } else {
    return equality_expr(p);
  }
}

// <declaration> = 'int' { '*' } <ident> ';'
static Ast *declaration(void) {
  // current token is 'int' when enter this function.
  CType *ctype = make_ctype(TYPE_INT, NULL);

  while (next_token()->type == TK_STAR)
    ctype = make_ctype(TYPE_PTR, ctype);

  expect_token(current_token(), TK_IDENT);

  Token *tk = current_token();
  Ast *p = make_ast_declaration(ctype, tk->text, tk);
  expect_token(next_token(), TK_SEMI);
  next_token();
  return p;
}

static Ast *statement(void);

// <selection_statement> = 'if' '(' <expression> ')' <statement>
//   [ 'else' <statement> ]
static Ast *selection_statement(void) {
  // current token is 'if' when enter this function.
  Ast *p = make_ast_statement(AST_IF_STATEMENT);

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

/*
  <iteration_statement> =
    'while' '(' <expr> ')' <statement> |
    'for' '(' <expr> ';' <expr> ';' <expr> ')' <statement>
*/
static Ast *iteration_statement(void) {
  // current token is 'while' or 'for' when enter this function.
  Ast *p;

  if (current_token()->type == TK_WHILE) {
    p = make_ast_statement(AST_WHILE_STATEMENT);

    expect_token(next_token(), TK_LPAR);
    next_token();
    p->cond = expr();
    expect_token(current_token(), TK_RPAR);
    next_token();
    p->statement = statement();
  } else {
    p = make_ast_statement(AST_FOR_STATEMENT);

    expect_token(next_token(), TK_LPAR);
    next_token();
    p->init = expr();
    expect_token(current_token(), TK_SEMI);
    next_token();
    p->cond = expr();
    expect_token(current_token(), TK_SEMI);
    next_token();
    p->step = expr();
    expect_token(current_token(), TK_RPAR);
    next_token();
    p->statement = statement();
  }

  return p;
}

// <compound_statement> = '{' { <declaration> | <statement> } '}'
static Ast *compound_statement(void) {
  // current token is '{' when enter this function.
  Ast *p = make_ast_compound_statement();

  next_token();
  while (current_token()->type != TK_RCUR) {
    if (current_token()->type == TK_INT)
      vector_push_back(p->statements, (void *)declaration());
    else
      vector_push_back(p->statements, (void *)statement());
  }
  next_token();

  return p;
}

static Ast *expr_statement(void) {
  Ast *p = expr();
  expect_token(current_token(), TK_SEMI);
  next_token();
  return p;
}

// <statement> = <selection_statement> | <iteration_statement> |
//   <compound_statement> | <expr_statement>
static Ast *statement(void) {
  if (current_token()->type == TK_IF)
    return selection_statement();
  else if (current_token()->type == TK_WHILE || current_token()->type == TK_FOR)
    return iteration_statement();
  else if (current_token()->type == TK_LCUR)
    return compound_statement();
  else
    return expr_statement();
}

// <decl_function> = 'int' <ident>
//   '(' [ 'int' { '*' } <ident> { ',' 'int' { '*' } <ident> } ] ')'
//   <compound_statement>
static Ast *decl_function(void) {
  // current token is 'int' when enter this function.
  Token *tk = next_token();
  expect_token(tk, TK_IDENT);

  Ast *p = make_ast_decl_func(tk->text, tk);
  expect_token(next_token(), TK_LPAR);

  if (second_token()->type != TK_RPAR) {
    do {
      expect_token(next_token(), TK_INT);
      CType *ctype = make_ctype(TYPE_INT, NULL);

      while (next_token()->type == TK_STAR)
        ctype = make_ctype(TYPE_PTR, ctype);

      expect_token(current_token(), TK_IDENT);
      vector_push_back(
          p->args, (void *)make_ast_declaration(ctype, current_token()->text,
                                                current_token()));
    } while (next_token()->type == TK_COMMA);
    expect_token(current_token(), TK_RPAR);
  } else
    next_token();

  expect_token(next_token(), TK_LCUR);
  p->statement = compound_statement();

  return p;
}

// <program> = { <decl_function> }
static Vector *program(void) {
  Vector *v = vector_new();

  while (current_token()->type != TK_EOF) {
    expect_token(current_token(), TK_INT);
    Ast *p = decl_function();
    vector_push_back(v, (void *)p);
  }

  return v;
}

static void semantic_analysis(Ast *p) {
  if (p == NULL)
    return;

  switch (p->type) {
    case AST_OP_ADD:
    case AST_OP_SUB:
    case AST_OP_MUL:
    case AST_OP_DIV:
    case AST_OP_LT:
    case AST_OP_LE:
    case AST_OP_GT:
    case AST_OP_GE:
    case AST_OP_EQUAL:
    case AST_OP_NEQUAL:
      semantic_analysis(p->left);
      semantic_analysis(p->right);
      p->ctype = p->left->ctype;
      break;
    case AST_OP_POST_INC:
    case AST_OP_POST_DEC:
    case AST_OP_PRE_INC:
    case AST_OP_PRE_DEC:
      if (p->left == NULL || p->left->type != AST_VAR)
        error_with_token(p->token, "expression is not assignable");
      p->ctype = p->left->ctype;
      break;
    case AST_OP_REF:
      semantic_analysis(p->left);
      p->ctype = make_ctype(TYPE_PTR, p->left->ctype);
      break;
    case AST_OP_DEREF:
      semantic_analysis(p->left);
      if (p->left->ctype->type != TYPE_PTR || p->left->ctype->ptrof == NULL)
        error_with_token(p->token, "indirection requires pointer operand");
      p->ctype = p->left->ctype->ptrof;
      break;
    case AST_OP_ASSIGN:
      semantic_analysis(p->left);
      semantic_analysis(p->right);
      if (p->left->type != AST_VAR && p->left->type != AST_OP_DEREF)
        error_with_token(p->token, "expression is not assignable");
      else if (p->left->ctype->type != p->right->ctype->type)
        error_with_token(p->token, "expression is not assignable");
      p->ctype = p->left->ctype;
      break;
    case AST_VAR:
      if (map_get(symbol_table, p->ident) == NULL)  // undefined variable.
        error_with_token(
            p->token, allocate_concat_3string("use of undeclared identifier '",
                                              p->ident, "'"));
      else
        p->ctype =
            ((SymbolTableEntry *)map_get(symbol_table, p->ident)->val)->ctype;
      break;
    case AST_DECLARATION: {
      SymbolTableEntry *_e =
          make_SymbolTableEntry(p->ctype, get_offset_from_bp(p->ctype->type));
      MapEntry *e = allocate_MapEntry(p->ident, (void *)_e);
      if (map_get(symbol_table, e->key) != NULL)  // already defined variable.
        error_with_token(p->token, allocate_concat_3string("redifinition of '",
                                                           p->ident, "'"));
      map_put(symbol_table, e);
      break;
    }
    case AST_CALL_FUNC:
      for (int i = p->args->size - 1; i >= 0; i--)
        semantic_analysis((Ast *)vector_at(p->args, i));
      break;
    case AST_DECL_FUNC:
      symbol_table = p->symbol_table;
      offset_from_bp = 0;
      if (p->args->size > 6)
        error_with_token(p->token, "too many arguments");
      for (int i = 0; i < p->args->size; i++)
        semantic_analysis((Ast *)vector_at(p->args, i));
      semantic_analysis(p->statement);
      p->offset_from_bp = offset_from_bp;
      break;
    case AST_COMPOUND_STATEMENT:
      for (int i = 0; i < p->statements->size; i++)
        semantic_analysis((Ast *)vector_at(p->statements, i));
      break;
    case AST_IF_STATEMENT:
      semantic_analysis(p->cond);
      semantic_analysis(p->left);
      semantic_analysis(p->right);
      break;
    case AST_FOR_STATEMENT:
      semantic_analysis(p->init);
      semantic_analysis(p->step);
    case AST_WHILE_STATEMENT:
      semantic_analysis(p->cond);
      semantic_analysis(p->statement);
      break;
  }
}

static void codegen(Ast *);

static void emit_lvalue(Ast *p) {
  if (p->type == AST_OP_DEREF) {
    codegen(p->left);
  } else if (p->type == AST_VAR) {
    printf("\tleaq %d(%%rbp), %%rax\n",
           -((SymbolTableEntry *)map_get(symbol_table, p->ident)->val)->offset);
    printf("\tpushq %%rax\n");
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
      emit_lvalue(p->left);
      printf("\tpopq %%rax\n");
      printf("\tpushq (%%rax)\n");
      printf("\t%s (%%rax)\n", p->type == AST_OP_POST_INC ? "incl" : "decl");
      break;
    case AST_OP_PRE_INC:
    case AST_OP_PRE_DEC:
      emit_lvalue(p->left);
      printf("\tpopq %%rax\n");
      printf("\t%s (%%rax)\n", p->type == AST_OP_PRE_INC ? "incl" : "decl");
      printf("\tpushq (%%rax)\n");
      break;
    case AST_OP_REF:
      emit_lvalue(p->left);
      break;
    case AST_OP_DEREF:
      codegen(p->left);
      printf("\tpopq %%rax\n");
      printf("\tpushq (%%rax)\n");
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
      emit_lvalue(p->left);
      codegen(p->right);
      printf("\tpopq %%rdi\n");
      printf("\tpopq %%rax\n");
      printf("\tmovl %%edi, (%%rax)\n");
      printf("\tpushq %%rdi\n");
      break;
    case AST_VAR:
      printf(
          "\tpushq %d(%%rbp)\n",
          -((SymbolTableEntry *)map_get(symbol_table, p->ident)->val)->offset);
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
      if (p->offset_from_bp > 0 && (p->offset_from_bp) % 16 == 0)
        printf("\tsub $%d, %%rsp\n", p->offset_from_bp);
      else if (p->symbol_table->size > 0)
        printf("\tsub $%d, %%rsp\n",
               (p->offset_from_bp) + (16 - p->offset_from_bp % 16));

      for (int i = 0; i < (p->args->size > 6 ? 6 : p->args->size); i++) {
        char *s = ((Ast *)vector_at(p->args, i))->ident;
        char *reg[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
        printf("\tmovl %%%s, %d(%%rbp)\n", reg[i],
               -((SymbolTableEntry *)map_get(symbol_table, s)->val)->offset);
      }

      codegen(p->statement);

      printf("\tpopq %%rax\n");
      printf("\tmovq %%rbp, %%rsp\n");
      printf("\tpopq %%rbp\n");
      printf("\tret\n");
      break;
    case AST_COMPOUND_STATEMENT:
      for (int i = 0; i < p->statements->size; i++)
        codegen((Ast *)vector_at(p->statements, i));
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
    case AST_WHILE_STATEMENT: {
      int seq1 = get_sequence_num(), seq2 = get_sequence_num();
      printf(".L%d:\n", seq1);
      codegen(p->cond);
      printf("\tpopq %%rax\n");
      printf("\ttest %%rax, %%rax\n");
      printf("\tjz .L%d\n", seq2);
      codegen(p->statement);
      printf("\tjmp .L%d\n", seq1);
      printf(".L%d:\n", seq2);
      break;
    }
    case AST_FOR_STATEMENT: {
      int seq1 = get_sequence_num(), seq2 = get_sequence_num();
      codegen(p->init);
      printf(".L%d:\n", seq1);
      codegen(p->cond);
      printf("\tpopq %%rax\n");
      printf("\ttest %%rax, %%rax\n");
      printf("\tjz .L%d\n", seq2);
      codegen(p->statement);
      codegen(p->step);
      printf("\tjmp .L%d\n", seq1);
      printf(".L%d:\n", seq2);
      break;
    }
  }
}

int main(void) {
  init_token_queue(stdin);
  Vector *v = program();

  for (int i = 0; i < v->size; i++)
    semantic_analysis((Ast *)vector_at(v, i));

  printf("\t.global main\n");
  for (int i = 0; i < v->size; i++)
    codegen((Ast *)vector_at(v, i));

  return 0;
}
