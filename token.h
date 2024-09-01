// Open_memstream is  POSIX function and its declaration isn't visible in the other program
// https://stackoverflow.com/questions/14862513/open-memstream-warning-pointer-from-integer-without-a-cast
#define _POSIX_C_SOURCE 200809L


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>

typedef struct Type Type;
typedef struct Node Node;
typedef struct Member Member;

// strings.c
char *format(char *fmt, ...);

// tokenizer.c 

typedef struct File {
  char *name; // Original filename
  int unique_id; // Unique file num
  char *contents;

  char *display_name; // Display name for error messages
  int line_number; // Line number delta
} File;


File *new_file(char* name, int file_num, char *contents);

typedef struct Token Token;

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
  TokenType token_type; // Kind of Token
  Token *next; // Next token
  int64_t val; // If tokenType == T_NUM, its value
  char *loc; // Token location
  int len; // Token length
  Type *type; // Used if T_STR
  char *str; // String literal contents including terminating '\0'

  int line_num;
} Token;

void error(char *fmt, ...);
void error_at(char *location, char *fmt, ...);
void error_tok(Token *token, char *fmt, ...);
bool equal(Token *token, char *op);
Token *skip(Token *token, char *op);
bool consume(Token **rest, Token *token, char *str);
Token *tokenize_file(char *filename);

#define unreachable() \
  error("internal error at %s:%d", __FILE__, __LINE__);

// parser.c

typedef struct Obj Obj;
struct Obj {
  Obj *next;
  char *name; // Variable name
  Type *type; // Type
  bool is_local; // local or global/function

  // Local variable
  int offset; // offset from %rbp

  // Global variable or function
  bool is_function;
  bool is_definition;

  // Global variable
  char *init_data;

  // Function;
  Obj *params;
  Node *body;
  Obj *locals;
  int stack_size;
};

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
  ND_COMMA, // ,
  ND_MEMBER, // . (struct member access)
  ND_ADDRESS, // unary &
  ND_DEREF, // unary *
  ND_RETURN, // "return"
  ND_IF, // "if" (expr)
  ND_FOR, // "for" or "while"
  ND_BLOCK, // {...}
  ND_FUNCALL, // Function call
  ND_STATEMENT, // Expression statement ";"
  ND_STATEMENT_EXPRESSION, // Statement expression [GNU] C extension
  ND_NULL_STATEMENT, // ";" with no actual expression or declaration
  ND_VAR, // Variable
  ND_NUM, // Integer
  ND_CAST, // Type cast
} NodeType;

// AST Node type
typedef struct Node {
  NodeType node_type; // Type of Node
  Node *next; // Next Node
  Token *token; // Representative token
  Type *type; // Type, like int or pointer to int or char, etc.
  
  Node *left; // left-side of AST
  Node *right; // right-side of AST 

  // "if" or "for" statement
  Node *cond;
  Node *then;
  Node *els;
  Node *init;
  Node *inc;

  // ND_BLOCK or statement expression
  Node *body;
  
  // Struct member access
  Member *member;

  // Function call
  char *funcname;
  Node *args;

  int64_t val; // Only used if NodeType == ND_NUM
  Obj *var; // Only used if NodeType == ND_VAR
} Node;

Node *new_cast(Node *expr, Type *type);
Obj *parse(Token *token);

// type.c

typedef enum {
  TY_VOID,
  TY_CHAR,
  TY_SHORT,
  TY_INT,
  TY_LONG,
  TY_PTR,
  TY_FUNC,
  TY_ARRAY,
  TY_STRUCT,
  TY_UNION,
} TypeKind;

struct Type {
  TypeKind kind;

  int size; // sizeof() value
  int align; // alignment
  
  // Pointer-to or array-of type
  // Intentionally use same member to represent pointer/array duality in C
  // In the case that a pointer is expected, we can look at this
  // member instead of "kind" member to determine whether a type is a pointer or not.
  // "Array of X" is naturally handled as if it were "pointer to X", as required by
  // C spec. 
  Type *base;

  // Declaration
  Token *name;

  // Array
  int array_len;

  // Struct
  Member *members;

  // Function type
  Type *return_type;
  Type *params;
  Type *next;
};

// struct
struct Member {
  Member *next;
  Type *type;
  Token *name;
  int offset;
};

extern Type *ty_void;

extern Type *ty_char;
extern Type *ty_short;
extern Type *ty_int;
extern Type *ty_long;

bool is_integer(Type *type);
Type *copy_type(Type *type);
Type *pointer_to(Type *base);
Type *func_type(Type *return_type);
Type *array_of(Type *base, int size);
void add_type(Node *node);

// asmgen.c

void gen_asm(Obj *program, FILE *out);
int align_to(int n, int align);


// Memory management
// When exiting the program, OS frees automatically
void free_node(Node *node); 

void free_token(Token *token);

void free_lvar(Obj *locals);

void free_memory(Token *token, Obj *program);

