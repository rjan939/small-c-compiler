#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <assert.h>
#include <stdbool.h>

typedef struct Token Token;
typedef struct Type Type;
typedef enum {
  T_IDENT,   // Identifiers
  T_PUNCT,   // Punctuators
  T_KEYWORD, // Keywords
  T_STR,     // String literals
  T_NUM,     // Numeric literals
  T_PP_NUM,  // Preprocessing numbers
  T_EOF,     // End-of-file markers
} TokenType;

typedef struct {
  char *name;
  int file_no;
  char *contents;

  // For #line directive
  char *display_name;
  int line_delta;
} File;

typedef struct Token {
    TokenType type;
    Token *next;
    int64_t val;
    long double fval;
    char *loc;
    int len;
    Type *ty;
    char *str;

    File *file;
    char *filename;
    int line_num;
    int line_delta;
    bool at_beginning;
    bool follows_space;
    Token *origin;
} Token;

typedef struct Type {
    TokenType type;
    int size;
    int align;
    bool is_unsigned;
    bool is_atomic;
    Type *origin;

    Type* base;

    Token *name;
    Token *name_pos;

    int array_len;
};