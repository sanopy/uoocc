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

// [a-zA-Z_] [0-9a-zA-Z_]*
static char *read_ident(FILE *fp, char c) {
  char ident[MAX_TOKEN_LENGTH];
  int idx = 0;

  if (!(isalpha(c) || c == '_')) {
    ungetc(c, fp);
    ident[idx] = '\0';
    return allocate_string(ident);
  }

  while (c != EOF) {
    if (!(isalpha(c) || isdigit(c) || c == '_')) {
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
  Token *t = malloc(sizeof(Token));

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

    if (c == '/') {
      char _c;
      if ((_c = getc(fp)) == '/') {  // skip liner comment
        while ((c = getc(fp)) != '\n')
          ;
        now_row++;
        now_col = 1;
        continue;
      } else if (_c == '*') {  // skip block comment
        Token *tk =
            make_token(now_row, now_col, TK_MISC, allocate_string("/*"));

        char c2;
        c = getc(fp);
        c2 = getc(fp);

        now_col += 2;
        if (c == '\n') {
          now_row++;
          now_col = 1;
        } else
          now_col++;
        if (c2 == '\n') {
          now_row++;
          now_col = 1;
        } else
          now_col++;

        while (!(c == '*' && c2 == '/') && c2 != EOF) {
          c = c2;
          c2 = getc(fp);
          if (c2 == '\n') {
            now_row++;
            now_col = 1;
          } else
            now_col++;
        }
        if (c2 == EOF)
          error_with_token(tk, "unterminated comment");
        continue;
      } else {
        ungetc(_c, fp);
      }
    }

    if (c == '+') {
      if ((c = getc(fp)) == '+') {
        now_col++;
        vector_push_back(
            v, make_token(now_row, now_col, TK_INC, allocate_string("++")));
      } else {
        ungetc(c, fp);
        vector_push_back(
            v, make_token(now_row, now_col, TK_PLUS, allocate_string("+")));
      }
    } else if (c == '-') {
      if ((c = getc(fp)) == '-') {
        now_col++;
        vector_push_back(
            v, make_token(now_row, now_col, TK_DEC, allocate_string("--")));
      } else if (c == '>') {
        now_col++;
        vector_push_back(
            v, make_token(now_row, now_col, TK_ARROW, allocate_string("->")));
      } else {
        ungetc(c, fp);
        vector_push_back(
            v, make_token(now_row, now_col, TK_MINUS, allocate_string("-")));
      }
    } else if (c == '*')
      vector_push_back(
          v, make_token(now_row, now_col, TK_STAR, allocate_string("*")));
    else if (c == '/')
      vector_push_back(
          v, make_token(now_row, now_col, TK_DIV, allocate_string("/")));
    else if (c == '&') {
      if ((c = getc(fp)) == '&') {
        now_col++;
        vector_push_back(
            v, make_token(now_row, now_col, TK_L_AND, allocate_string("&&")));
      } else {
        ungetc(c, fp);
        vector_push_back(
            v, make_token(now_row, now_col, TK_AMP, allocate_string("&")));
      }
    } else if (c == '|') {
      if ((c = getc(fp)) == '|') {
        now_col++;
        vector_push_back(
            v, make_token(now_row, now_col, TK_L_OR, allocate_string("||")));
      } else {
        ungetc(c, fp);
        vector_push_back(
            v, make_token(now_row, now_col, TK_B_OR, allocate_string("|")));
      }
    } else if (c == '^')
      vector_push_back(
          v, make_token(now_row, now_col, TK_B_XOR, allocate_string("^")));
    else if (c == '~')
      vector_push_back(
          v, make_token(now_row, now_col, TK_B_NOT, allocate_string("~")));
    else if (c == '(')
      vector_push_back(
          v, make_token(now_row, now_col, TK_LPAR, allocate_string("(")));
    else if (c == ')')
      vector_push_back(
          v, make_token(now_row, now_col, TK_RPAR, allocate_string(")")));
    else if (c == '=') {
      if ((c = getc(fp)) == '=') {
        now_col++;
        vector_push_back(
            v, make_token(now_row, now_col, TK_EQUAL, allocate_string("==")));
      } else {
        ungetc(c, fp);
        vector_push_back(
            v, make_token(now_row, now_col, TK_ASSIGN, allocate_string("=")));
      }
    } else if (c == ';')
      vector_push_back(
          v, make_token(now_row, now_col, TK_SEMI, allocate_string(";")));
    else if (c == ',')
      vector_push_back(
          v, make_token(now_row, now_col, TK_COMMA, allocate_string(",")));
    else if (c == '.')
      vector_push_back(
          v, make_token(now_row, now_col, TK_DOT, allocate_string(".")));
    else if (c == '{')
      vector_push_back(
          v, make_token(now_row, now_col, TK_LCUR, allocate_string("{")));
    else if (c == '}')
      vector_push_back(
          v, make_token(now_row, now_col, TK_RCUR, allocate_string("}")));
    else if (c == '[')
      vector_push_back(
          v, make_token(now_row, now_col, TK_LBRA, allocate_string("[")));
    else if (c == ']')
      vector_push_back(
          v, make_token(now_row, now_col, TK_RBRA, allocate_string("]")));
    else if (c == '<') {
      if ((c = getc(fp)) == '<') {
        now_col++;
        vector_push_back(
            v, make_token(now_row, now_col, TK_LSHIFT, allocate_string("<<")));
      } else if (c == '=') {
        now_col++;
        vector_push_back(
            v, make_token(now_row, now_col, TK_LE, allocate_string("<=")));
      } else {
        ungetc(c, fp);
        vector_push_back(
            v, make_token(now_row, now_col, TK_LT, allocate_string("<")));
      }
    } else if (c == '>') {
      if ((c = getc(fp)) == '>') {
        now_col++;
        vector_push_back(
            v, make_token(now_row, now_col, TK_RSHIFT, allocate_string(">>")));
      } else if (c == '=') {
        now_col++;
        vector_push_back(
            v, make_token(now_row, now_col, TK_GE, allocate_string(">=")));
      } else {
        ungetc(c, fp);
        vector_push_back(
            v, make_token(now_row, now_col, TK_GT, allocate_string(">")));
      }
    } else if (c == '!') {
      if ((c = getc(fp)) == '=') {
        now_col++;
        vector_push_back(
            v, make_token(now_row, now_col, TK_NEQUAL, allocate_string("!=")));
      } else {
        ungetc(c, fp);
        vector_push_back(
            v, make_token(now_row, now_col, TK_L_NOT, allocate_string("!")));
      }
    } else if (isdigit(c)) {
      char *s = read_number(fp, c);
      vector_push_back(v, make_token(now_row, now_col, TK_NUM, s));
      now_col += strlen(s) - 1;
    } else if (isalpha(c) || c == '_') {
      char *s = read_ident(fp, c);
      if (strcmp(s, "sizeof") == 0)
        vector_push_back(v, make_token(now_row, now_col, TK_SIZEOF, s));
      else if (strcmp(s, "if") == 0)
        vector_push_back(v, make_token(now_row, now_col, TK_IF, s));
      else if (strcmp(s, "else") == 0)
        vector_push_back(v, make_token(now_row, now_col, TK_ELSE, s));
      else if (strcmp(s, "while") == 0)
        vector_push_back(v, make_token(now_row, now_col, TK_WHILE, s));
      else if (strcmp(s, "for") == 0)
        vector_push_back(v, make_token(now_row, now_col, TK_FOR, s));
      else if (strcmp(s, "int") == 0)
        vector_push_back(v, make_token(now_row, now_col, TK_INT, s));
      else if (strcmp(s, "char") == 0)
        vector_push_back(v, make_token(now_row, now_col, TK_CHAR, s));
      else if (strcmp(s, "void") == 0)
        vector_push_back(v, make_token(now_row, now_col, TK_VOID, s));
      else if (strcmp(s, "return") == 0)
        vector_push_back(v, make_token(now_row, now_col, TK_RETURN, s));
      else if (strcmp(s, "enum") == 0)
        vector_push_back(v, make_token(now_row, now_col, TK_ENUM, s));
      else if (strcmp(s, "struct") == 0)
        vector_push_back(v, make_token(now_row, now_col, TK_STRUCT, s));
      else if (strcmp(s, "typedef") == 0)
        vector_push_back(v, make_token(now_row, now_col, TK_TYPEDEF, s));
      else if (strcmp(s, "break") == 0)
        vector_push_back(v, make_token(now_row, now_col, TK_BREAK, s));
      else if (strcmp(s, "continue") == 0)
        vector_push_back(v, make_token(now_row, now_col, TK_CONTINUE, s));
      else
        vector_push_back(v, make_token(now_row, now_col, TK_IDENT, s));
      now_col += strlen(s) - 1;
    } else if (c == '"') {
      int row = now_row, col = now_col;
      int len = 0;
      char str[MAX_TOKEN_LENGTH];
      str[len++] = c;
      now_col++;
      while ((c = getc(fp)) != '"') {
        if (c == '\n') {
          now_row++;
          now_col = 1;
        } else
          now_col++;

        str[len++] = c;
      }
      str[len++] = c;
      str[len] = '\0';
      now_col++;
      vector_push_back(v, make_token(row, col, TK_STR, allocate_string(str)));
    } else
      error("unknown token has read");

    now_col++;
  }
  vector_push_back(
      v, make_token(now_row, now_col, TK_EOF, allocate_string("EOF")));
}

Token *current_token(void) {
  return vector_at(token_queue.vec, token_queue.idx);
}

Token *second_token(void) {
  return vector_at(token_queue.vec, token_queue.idx + 1);
}

Token *third_token(void) {
  return vector_at(token_queue.vec, token_queue.idx + 2);
}

Token *next_token(void) {
  return vector_at(token_queue.vec, ++token_queue.idx);
}
