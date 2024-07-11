// Open_memstream is  POSIX function and its declaration isn't visible in the other program
// https://stackoverflow.com/questions/14862513/open-memstream-warning-pointer-from-integer-without-a-cast
#define _POSIX_C_SOURCE 200809L


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>

typedef struct File {
  char *name; // Original filename
  int unique_id; // Unique file num
  char *contents;

  char *display_name; // Display name for error messages
  int line_number; // Line number delta
} File;


File *new_file(char* name, int file_num, char *contents);

typedef struct Token Token;
typedef struct Type Type;

typedef enum TokenType {
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
  int val;
  char *loc;
  int len;
} Token;

void error(char *fmt, ...);
void error_at(char *location, char *fmt, ...);
void error_tok(Token *token, char *fmt, ...);
bool equal(Token *token, char *op);
Token *skip(Token *token, char *op);
Token *tokenize(char *input);

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


// parser.c

typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_NEG, // unary -
  ND_EQ, // --
  ND_NE, // !=
  ND_LT, // < or >
  ND_LE, // <= or >=
  ND_ASSIGN, // =
  ND_STATEMENT, // Expression statement ";"
  ND_VAR, // Variable
  ND_NUM, // Integer
} NodeType;

// AST Node type
typedef struct Node {
  NodeType type; // Type of Node
  struct Node *next; // Next Node
  struct Node *left; // left-side of AST
  struct Node *right; // right-side of AST
  int val; // Only used if type == ND_NUM
} Node;

Node *parse(Token *token);

// asmgen.c

void gen_asm(Node *node);


// Memory management
// When exiting the program, OS frees automatically
void free_node(Node *node); 

void free_token(Token *token);

void free_memory(Token *token, Node *node);

