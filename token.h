#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

File *new_file(char* name, int file_num, char *contents);
static char *read_file(char* path);

typedef struct Token Token;
typedef struct Type Type;

typedef struct File {
  char *name; // Original filename
  int unique_id; // Unique file num
  char *contents;

  char *display_name; // Display name for error messages
  int line_number; // Line number delta
} File;

typedef enum {
  T_IDENT,   // Identifiers
  T_PUNCT,   // Punctuators
  T_KEYWORD, // Keywords
  T_STR,     // String literals
  T_NUM,     // Numeric literals
  T_PP_NUM,  // Preprocessing numbers
  T_EOF,     // End-of-file markers
} TokenType;

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
} Type;