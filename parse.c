#include <stdlib.h>
#include "uoocc.h"

Map *string_table;

CType *make_ctype(int type, CType *ptrof) {
  CType *p = (CType *)malloc(sizeof(CType));
  p->type = type;
  p->ptrof = ptrof;
  p->array_size = 0;
  return p;
}

Ast *make_ast_op(int type, Ast *left, Ast *right, Token *token) {
  Ast *p = malloc(sizeof(Ast));
  p->type = type;
  p->left = left;
  p->right = right;
  p->token = token;
  return p;
}

Ast *make_ast_int(int val) {
  Ast *p = malloc(sizeof(Ast));
  p->type = AST_INT;
  p->ctype = make_ctype(TYPE_INT, NULL);
  p->ival = val;
  return p;
}

Ast *make_ast_str(int label) {
  Ast *p = malloc(sizeof(Ast));
  p->type = AST_STR;
  p->ctype = make_ctype(TYPE_PTR, make_ctype(TYPE_CHAR, NULL));
  p->label = label;
  return p;
}

static Ast *make_ast_var(char *ident, Token *token) {
  Ast *p = malloc(sizeof(Ast));
  p->type = AST_VAR;
  p->ident = ident;
  p->token = token;
  return p;
}

static Ast *make_ast_decl_var(CType *ctype, char *ident, Token *token) {
  Ast *p = malloc(sizeof(Ast));
  p->type = AST_DECL_LOCAL_VAR;
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
  p->symbol_table =
      map_new(symbol_table);  // TODO: it is never express local scope.
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
//   | <string>
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
  } else if (type == TK_STR) {
    MapEntry *e = map_get(string_table, current_token()->text);
    if (e == NULL) {
      int seq = get_sequence_num();
      e = allocate_MapEntry(current_token()->text, allocate_integer(seq));
      map_put(string_table, e);
    }
    next_token();
    ret = make_ast_str(*(int *)e->val);
  } else
    error_with_token(current_token(), "primary-expression was expected");

  return ret;
}

/*
  <postfix_expr> = <primary_expr> <postfix_expr_tail>
  <postfix_expr_tail> = ε |
    '++' <postfix_expr_tail> |
    '--' <postfix_expr_tail> |
    '[' <expr> ']' <postfix_expr_tail>
*/
static Ast *postfix_expr_tail(Ast *left) {
  Token *tk = current_token();
  int type = tk->type;
  if (type == TK_INC || type == TK_DEC) {
    next_token();
    Ast *right = postfix_expr_tail(NULL);
    int ast_op = type == TK_INC ? AST_OP_POST_INC : AST_OP_POST_DEC;
    Ast *p = make_ast_op(ast_op, left, right, tk);
    return postfix_expr_tail(p);
  } else if (type == TK_LBRA) {
    next_token();
    Ast *p = make_ast_op(AST_SUBSCRIPT, left, expr(), tk);
    expect_token(current_token(), TK_RBRA);
    next_token();
    return postfix_expr_tail(p);
  } else {
    return left;
  }
}

static Ast *postfix_expr(void) {
  Ast *p = primary_expr();
  return postfix_expr_tail(p);
}

static int is_type_specifier(Token *);
static CType *type_name(void);

/*
  <unary_expr> = <postfix_expr> | '++' <unary_expr> | '--' <unary_expr> |
    '&' <unary_expr> | '*' <unary_expr> | '~' <unary_expr> | '!' <unary_expr> |
    'sizeof' <unary_expr> | 'sizeof' '(' <type_name> ')'
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
  else if (type == TK_B_NOT)
    ast_op = AST_OP_B_NOT;
  else if (type == TK_L_NOT)
    ast_op = AST_OP_L_NOT;
  else if (type == TK_SIZEOF)
    ast_op = AST_OP_SIZEOF;
  else
    ast_op = -1;

  if (ast_op == AST_OP_SIZEOF && second_token()->type == TK_LPAR &&
      is_type_specifier(third_token())) {
    next_token();
    next_token();
    CType *ctype = type_name();
    expect_token(current_token(), TK_RPAR);
    next_token();
    return postfix_expr_tail(make_ast_int(sizeof_ctype(ctype)));
  } else if (ast_op != -1) {
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
  <shift_expr> = <additive_expr> <shift_expr_tail>
  <shift_expr_tail> = ε | '<<' <additive_expr> <shift_expr_tail> |
    '>>' <additive_expr> <shift_expr_tail>
*/
static Ast *shift_expr_tail(Ast *left) {
  Token *tk = current_token();
  int type = tk->type;
  if (type == TK_LSHIFT || type == TK_RSHIFT) {
    next_token();
    Ast *right = additive_expr(NULL);
    int ast_op = type == TK_LSHIFT ? AST_OP_LSHIFT : AST_OP_RSHIFT;
    Ast *p = make_ast_op(ast_op, left, right, tk);
    return shift_expr_tail(p);
  } else {
    return left;
  }
}

static Ast *shift_expr(Ast *unary) {
  Ast *p = additive_expr(unary);
  return shift_expr_tail(p);
}

/*
  <relational_expr> = <shift_expr> <relational_expr_tail>
  <relational_expr_tail> = ε | '<' <shift_expr> <relational_expr_tail> |
    '<=' <shift_expr> <relational_expr_tail> |
    '>'  <shift_expr> <relational_expr_tail> |
    '>=' <shift_expr> <relational_expr_tail>
*/
static Ast *relational_expr_tail(Ast *left) {
  Token *tk = current_token();
  int type = tk->type;
  if (type == TK_LT || type == TK_LE || type == TK_GT || type == TK_GE) {
    next_token();
    Ast *p, *right = shift_expr(NULL);
    if (type == TK_LT)
      p = make_ast_op(AST_OP_LT, left, right, tk);
    else if (type == TK_LE)
      p = make_ast_op(AST_OP_LE, left, right, tk);
    else if (type == TK_GT)  // a > b <=> b < a
      p = make_ast_op(AST_OP_LT, right, left, tk);
    else  // a >= b <=> b <= a
      p = make_ast_op(AST_OP_LE, right, left, tk);
    return relational_expr_tail(p);
  } else {
    return left;
  }
}

static Ast *relational_expr(Ast *unary) {
  Ast *p = shift_expr(unary);
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

/*
  <and_expr> = <equality_expr> <and_expr_tail>
  <and_expr_tail> = ε | '&' <equality_expr> <and_expr_tail>
*/
static Ast *and_expr_tail(Ast *left) {
  Token *tk = current_token();
  if (tk->type == TK_AMP) {
    next_token();
    Ast *right = relational_expr(NULL);
    Ast *p = make_ast_op(AST_OP_B_AND, left, right, tk);
    return and_expr_tail(p);
  } else {
    return left;
  }
}

static Ast *and_expr(Ast *unary) {
  Ast *p = equality_expr(unary);
  return and_expr_tail(p);
}

/*
  <exclusive_or_expr> = <and_expr> <exclusive_or_expr_tail>
  <exclusive_or_expr_tail> = ε | '^' <and_expr> <exclusive_or_expr_tail>
*/
static Ast *exclusive_or_expr_tail(Ast *left) {
  Token *tk = current_token();
  if (tk->type == TK_B_XOR) {
    next_token();
    Ast *right = and_expr(NULL);
    Ast *p = make_ast_op(AST_OP_B_XOR, left, right, tk);
    return exclusive_or_expr_tail(p);
  } else {
    return left;
  }
}

static Ast *exclusive_or_expr(Ast *unary) {
  Ast *p = and_expr(unary);
  return exclusive_or_expr_tail(p);
}

/*
  <inclusive_or_expr> = <exclusive_or_expr> <inclusive_or_expr_tail>
  <inclusive_or_expr_tail> = ε | '|' <exclusive_or_expr>
    <inclusive_or_expr_tail>
*/
static Ast *inclusive_or_expr_tail(Ast *left) {
  Token *tk = current_token();
  if (tk->type == TK_B_OR) {
    next_token();
    Ast *right = exclusive_or_expr(NULL);
    Ast *p = make_ast_op(AST_OP_B_OR, left, right, tk);
    return inclusive_or_expr_tail(p);
  } else {
    return left;
  }
}

static Ast *inclusive_or_expr(Ast *unary) {
  Ast *p = exclusive_or_expr(unary);
  return inclusive_or_expr_tail(p);
}

/*
  <logical_and_expr> = <inclusive_or_expr> <logical_and_expr_tail>
  <logical_and_expr_tail> = ε | '&&' <inclusive_or_expr>
    <logical_and_expr_tail>
*/
static Ast *logical_and_expr_tail(Ast *left) {
  Token *tk = current_token();
  if (tk->type == TK_L_AND) {
    next_token();
    Ast *right = inclusive_or_expr(NULL);
    Ast *p = make_ast_op(AST_OP_L_AND, left, right, tk);
    return logical_and_expr_tail(p);
  } else {
    return left;
  }
}

static Ast *logical_and_expr(Ast *unary) {
  Ast *p = inclusive_or_expr(unary);
  return logical_and_expr_tail(p);
}

/*
  <logical_or_expr> = <logical_and_expr> <logical_or_expr_tail>
  <logical_or_expr_tail> = ε | '||' <logical_and_expr>
    <logical_or_expr_tail>
*/
static Ast *logical_or_expr_tail(Ast *left) {
  Token *tk = current_token();
  if (tk->type == TK_L_OR) {
    next_token();
    Ast *right = logical_and_expr(NULL);
    Ast *p = make_ast_op(AST_OP_L_OR, left, right, tk);
    return logical_or_expr_tail(p);
  } else {
    return left;
  }
}

static Ast *logical_or_expr(Ast *unary) {
  Ast *p = logical_and_expr(unary);
  return logical_or_expr_tail(p);
}

// <expr> = <unary_expr> '=' <expr> | <logical_or_expr>
static Ast *expr(void) {
  Ast *p = unary_expr();
  if (current_token()->type == TK_ASSIGN) {
    Token *tk = current_token();
    next_token();
    return make_ast_op(AST_OP_ASSIGN, p, expr(), tk);
  } else {
    return logical_or_expr(p);
  }
}

// <pointer> = '*' | '*' <pointer>
static CType *pointer(CType *ctype) {
  // current token is '*' when enter this function.
  ctype = make_ctype(TYPE_PTR, ctype);
  if (next_token()->type == TK_STAR)
    return pointer(ctype);
  else
    return ctype;
}

static Ast *decl_function(Token *);

// <direct_declarator_tail> = ε | '[' <number> ']' <direct_declarator_tail> |
//   <decl_function>
static Ast *direct_declarator_tail(Token *ident, CType *ctype) {
  if (current_token()->type == TK_LBRA) {
    expect_token(next_token(), TK_NUM);
    if (ctype->type != TYPE_ARRAY) {
      ctype = make_ctype(TYPE_ARRAY, ctype);
      ctype->array_size = current_token()->number;
    } else {
      CType *p = ctype, *q = ctype->ptrof;
      while (q->type == TYPE_ARRAY) {
        p = q;
        q = q->ptrof;
      }
      CType *new = make_ctype(TYPE_ARRAY, q);
      new->array_size = current_token()->number;
      p->ptrof = new;
    }
    expect_token(next_token(), TK_RBRA);
    next_token();
    return direct_declarator_tail(ident, ctype);
  } else if (current_token()->type == TK_LPAR) {
    return decl_function(ident);
  } else
    return make_ast_decl_var(ctype, ident->text, ident);
}

// <direct_declarator> = <ident> <direct_declarator_tail>
static Ast *direct_declarator(CType *ctype) {
  expect_token(current_token(), TK_IDENT);
  Token *tk = current_token();
  next_token();
  return direct_declarator_tail(tk, ctype);
}

// <declarator> = <pointer_opt> <direct_declarator>
static Ast *declarator(CType *ctype) {
  if (current_token()->type == TK_STAR)
    ctype = pointer(ctype);
  return direct_declarator(ctype);
}

static int is_type_specifier(Token *tk) {
  int t = tk->type;
  return t == TK_INT || t == TK_CHAR;
}

// <type_specifier> = 'int' | 'char'
static CType *type_specifier(void) {
  // current token is 'int' or 'char' when enter this function.
  CType *ret;
  if (current_token()->type == TK_INT)
    ret = make_ctype(TYPE_INT, NULL);
  else
    ret = make_ctype(TYPE_CHAR, NULL);

  next_token();
  return ret;
}

// <declaration_specifiers> = <type_specifier>
static CType *declaration_specifiers(void) {
  // current token is 'int' or 'char' when enter this function.
  return type_specifier();
}

// <declaration> = <declaration_specifiers> <declarator> ';'
static Ast *declaration(void) {
  // current token is 'int' or 'char' when enter this function.
  Ast *p = declarator(declaration_specifiers());

  expect_token(current_token(), TK_SEMI);
  next_token();

  return p;
}

// <type_name> = <type_specifier> <pointer_opt>
static CType *type_name(void) {
  CType *ret = type_specifier();
  if (current_token()->type == TK_STAR)
    ret = pointer(ret);
  return ret;
}

// <decl_function> =
//   '(' [ <declaration_specifiers> <pointer_opt> <ident>
//   { ',' <declaration_specifiers> <pointer_opt> <ident> } ] ')'
static Ast *decl_function(Token *tk) {
  // current token is '(' when enter this function.
  Ast *p = make_ast_decl_func(tk->text, tk);

  if (second_token()->type != TK_RPAR) {
    do {
      next_token();
      if (!is_type_specifier(current_token()))
        error_with_token(current_token(), "type_specifier was expected");
      CType *ctype = declaration_specifiers();

      if (current_token()->type == TK_STAR)
        ctype = pointer(ctype);

      expect_token(current_token(), TK_IDENT);
      vector_push_back(p->args, make_ast_decl_var(ctype, current_token()->text,
                                                  current_token()));
    } while (next_token()->type == TK_COMMA);
    expect_token(current_token(), TK_RPAR);
  } else
    next_token();

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
    if (is_type_specifier(current_token()))
      vector_push_back(p->statements, declaration());
    else
      vector_push_back(p->statements, statement());
  }
  next_token();

  return p;
}

// <expr_statement> = <expr_opt> ';'
static Ast *expr_statement(void) {
  if (current_token()->type != TK_SEMI) {
    Ast *p = make_ast_statement(AST_EXPR_STATEMENT);
    p->expr = expr();
    expect_token(current_token(), TK_SEMI);
    next_token();
    return p;
  } else {
    next_token();
    return NULL;
  }
}

// <expr_statement> = 'return' <expr_opt> ';'
static Ast *jump_statement(void) {
  // current token is 'return' when enter this function.
  Ast *p = make_ast_statement(AST_RETURN_STATEMENT);
  next_token();
  if (current_token()->type != TK_SEMI) {
    p->expr = expr();
    expect_token(current_token(), TK_SEMI);
    next_token();
  } else {
    next_token();
    p->expr = NULL;
  }
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
  else if (current_token()->type == TK_RETURN)
    return jump_statement();
  else
    return expr_statement();
}

// <program> = { <declaration_specifiers> <declarator> ';' |
//   <declaration_specifiers> <declarator> <compound_statement> }
Vector *program(void) {
  Vector *v = vector_new();

  while (current_token()->type != TK_EOF) {
    if (!is_type_specifier(current_token()))
      error_with_token(current_token(), "type_specifier was expected");
    Ast *p = declarator(declaration_specifiers());
    if (p->type == AST_DECL_LOCAL_VAR) {
      p->type = AST_DECL_GLOBAL_VAR;
      expect_token(current_token(), TK_SEMI);
      next_token();
    } else if (p->type == AST_DECL_FUNC) {
      expect_token(current_token(), TK_LCUR);
      p->statement = compound_statement();
    } else {
      error("declaration for variable or function was expected");
    }
    vector_push_back(v, p);
  }

  return v;
}
