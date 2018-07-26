#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uoocc.h"

TokenQueue token_queue;

// [0-9]+
static char *read_number(FILE *fp, char c) {
  char num[MAX_TOKEN_LENGTH];
  int idx = 0;

  while (c != EOF) {
    if (!isdigit(c)) {
      ungetc(c, fp);
      break;
    }
    num[idx++] = c;
    c = getc(fp);
  }

  num[idx] = '\0';
  return allocate_string(num);
}

// [a-zA-Z_]+
static char *read_ident(FILE *fp, char c) {
  char ident[MAX_TOKEN_LENGTH];
  int idx = 0;

  while (c != EOF) {
    if (!(isalpha(c) || c == '_')) {
      ungetc(c, fp);
      break;
    }
    ident[idx++] = c;
    c = getc(fp);
  }

  ident[idx] = '\0';
  return allocate_string(ident);
}

static Token *make_token(int row, int col, int type, char *text) {
  Token *t = (Token *)malloc(sizeof(Token));

  t->row = row;
  t->col = col;
  t->type = type;
  t->text = text;

  if (t->type == TK_NUM)
    t->number = atoi(text);

  return t;
}

void init_token_queue(FILE *fp) {
  token_queue.vec = vector_new();
  token_queue.idx = 0;

  if (fp == NULL)
    return;

  char c;
  Vector *v = token_queue.vec;
  int now_row, now_col;
  now_row = now_col = 1;
  while ((c = getc(fp)) != EOF) {
    if (c == '\n') {
      now_row++;
      now_col = 1;
      continue;
    } else if (isspace(c)) {
      now_col++;
      continue;
    }

    if (c == '+')
      vector_push_back(v, (void *)make_token(now_row, now_col, TK_PLUS,
                                             allocate_string("+")));
    else if (c == '-')
      vector_push_back(v, (void *)make_token(now_row, now_col, TK_MINUS,
                                             allocate_string("-")));
    else if (c == '*')
      vector_push_back(v, (void *)make_token(now_row, now_col, TK_MUL,
                                             allocate_string("*")));
    else if (c == '/')
      vector_push_back(v, (void *)make_token(now_row, now_col, TK_DIV,
                                             allocate_string("/")));
    else if (c == '(')
      vector_push_back(v, (void *)make_token(now_row, now_col, TK_LPAR,
                                             allocate_string("(")));
    else if (c == ')')
      vector_push_back(v, (void *)make_token(now_row, now_col, TK_RPAR,
                                             allocate_string(")")));
    else if (c == '=') {
      if ((c = getc(fp)) == '=') {
        now_col++;
        vector_push_back(v, (void *)make_token(now_row, now_col, TK_EQUAL,
                                               allocate_string("==")));
      } else {
        ungetc(c, fp);
        vector_push_back(v, (void *)make_token(now_row, now_col, TK_ASSIGN,
                                               allocate_string("=")));
      }
    } else if (c == ';')
      vector_push_back(v, (void *)make_token(now_row, now_col, TK_SEMI,
                                             allocate_string(";")));
    else if (c == ',')
      vector_push_back(v, (void *)make_token(now_row, now_col, TK_COMMA,
                                             allocate_string(",")));
    else if (c == '{')
      vector_push_back(v, (void *)make_token(now_row, now_col, TK_LCUR,
                                             allocate_string("{")));
    else if (c == '}')
      vector_push_back(v, (void *)make_token(now_row, now_col, TK_RCUR,
                                             allocate_string("}")));
    else if (c == '!') {
      if ((c = getc(fp)) == '=') {
        now_col++;
        vector_push_back(v, (void *)make_token(now_row, now_col, TK_NEQUAL,
                                               allocate_string("!=")));
      } else {
        ungetc(c, fp);
      }
    } else if (isdigit(c)) {
      char *s = read_number(fp, c);
      vector_push_back(v, (void *)make_token(now_row, now_col, TK_NUM, s));
      now_col += strlen(s) - 1;
    } else if (isalpha(c) || c == '_') {
      char *s = read_ident(fp, c);
      vector_push_back(v, (void *)make_token(now_row, now_col, TK_IDENT, s));
      now_col += strlen(s) - 1;
    } else
      error("unknown token has read");

    now_col++;
  }
  vector_push_back(
      v, (void *)make_token(now_row, now_col, TK_EOF, allocate_string("EOF")));
}

Token *current_token(void) {
  return (Token *)vector_at(token_queue.vec, token_queue.idx);
}

Token *second_token(void) {
  return (Token *)vector_at(token_queue.vec, token_queue.idx + 1);
}

Token *next_token(void) {
  return (Token *)vector_at(token_queue.vec, ++token_queue.idx);
}
