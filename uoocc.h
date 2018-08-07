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

typedef struct {
  Vector *vec;
  int size;
} Map;

extern Map *symbol_table;

MapEntry *allocate_MapEntry(char *, void *);
Map *map_new(void);
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
