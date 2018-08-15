#include <stdio.h>

// vector.c
typedef struct {
  void **data;
  int size;
  int reserved_size;
} Vector;

#define VECTOR_BLOCK_SIZE 4

Vector *vector_new(void);
int vector_push_back(Vector *, void *);
void *vector_at(Vector *, int);

// map.c
typedef struct {
  char *key;
  void *val;
} MapEntry;

typedef struct _Map {
  Vector *vec;
  int size;
  struct _Map *next;
} Map;

extern Map *symbol_table;

MapEntry *allocate_MapEntry(char *, void *);
Map *map_new(Map *);
int map_put(Map *, MapEntry *);
MapEntry *map_get(Map *, char *);

// lex.c
#define MAX_TOKEN_LENGTH 256

enum {
  TK_EOF,
  TK_NUM,
  TK_IDENT,
  TK_PLUS,    // +
  TK_MINUS,   // -
  TK_STAR,    // *
  TK_DIV,     // /
  TK_AMP,     // &
  TK_LPAR,    // (
  TK_RPAR,    // )
  TK_ASSIGN,  // =
  TK_SEMI,    // ;
  TK_COMMA,   // ,
  TK_LCUR,    // {
  TK_RCUR,    // }
  TK_LBRA,    // [
  TK_RBRA,    // ]
  TK_INC,     // ++
  TK_DEC,     // --
  TK_LT,      // <
  TK_LE,      // <=
  TK_GT,      // >
  TK_GE,      // >=
  TK_EQUAL,   // ==
  TK_NEQUAL,  // !=
  TK_IF,      // if
  TK_ELSE,    // else
  TK_WHILE,   // while
  TK_FOR,     // for
  TK_INT,     // int
};

typedef struct {
  int row;
  int col;
  int type;
  char *text;
  int number;
} Token;

typedef struct {
  Vector *vec;
  int idx;
} TokenQueue;

extern TokenQueue token_queue;

void init_token_queue(FILE *);
Token *current_token(void);
Token *second_token(void);
Token *next_token(void);

// mylib.c
char *allocate_string(char *);
char *allocate_concat_2string(char *, char *);
char *allocate_concat_3string(char *, char *, char *);
int *allocate_integer(int);
void error(char *);
void error_with_token(Token *, char *);
void expect_token(Token *, int);
int get_sequence_num(void);

// parse.c
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
  AST_OP_EQUAL,
  AST_OP_NEQUAL,
  AST_OP_ASSIGN,
  AST_VAR,
  AST_DECL_LOCAL_VAR,
  AST_DECL_GLOBAL_VAR,
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
  TYPE_ARRAY,
};

typedef struct _CType {
  int type;
  struct _CType *ptrof;
  int array_size;
} CType;

typedef struct {
  CType *ctype;
  int offset;
  int is_global;
  char *ident;
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
  SymbolTableEntry *symbol_table_entry;
  struct _Ast *left;
  struct _Ast *right;
  struct _Ast *cond;
  struct _Ast *init;
  struct _Ast *step;
  struct _Ast *statement;
} Ast;

CType *make_ctype(int, CType *);
Ast *make_ast_op(int, Ast *, Ast *, Token *);
Ast *make_ast_int(int);
Vector *program(void);

// analyze.c
SymbolTableEntry *symboltable_get(Map *, char *);
void semantic_analysis(Ast *);
int sizeof_ctype(CType *);

// gen.c
void codegen(Ast *);
