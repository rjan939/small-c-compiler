#include "token.h"
#include <stdarg.h>
#include <assert.h>

static char *current_input;

static void error(char *fmt, ...) {
  va_list argument_pointer;
  va_start(argument_pointer, fmt);
  vfprintf(stderr, fmt, argument_pointer);
  fprintf(stderr, "\n");
  exit(1);
}

// Reports error location and then exits
static void verror_at(char *location, char *fmt, va_list argument_pointer) {
  int position = location - current_input;
  fprintf(stderr, "%s\n", current_input);
  fprintf(stderr, "%*s", position, "");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, argument_pointer);
  fprintf(stderr, "\n");
  exit(1);
}

static void error_at(char *location, char *fmt, ...) {
  va_list argument_pointer;
  va_start(argument_pointer, fmt);
  verror_at(location, fmt, argument_pointer);
}

static void error_tok(Token *token, char *fmt, ...) {
  va_list argument_pointer;
  va_start(argument_pointer, fmt);
  verror_at(token->loc, fmt, argument_pointer);
}

static bool equal(Token *token, char *op) {
  return memcmp(token->loc, op, token->len) == 0 && op[token->len] == '\0';
}

static Token *skip(Token *token, char *s) {
  if (!equal(token, s))
    error_tok(token, "expected '%s'", s);
  return token->next;
}

int get_number(Token *token) {
  if (token->type != T_NUM) error_tok(token, "expected a number");
  return token->val;
}


static Token *new_token(TokenType type, char *start, char *end) {
  Token *token = calloc(1, sizeof(Token));
  if (token == NULL) error("not enough memory in system");
  token->type = type;
  token->loc = start;
  token->len = end - start;
  return token;
}

static bool startswith(char *p, char *q) {
  return strncmp(p, q, strlen(q)) == 0;
}

static int get_punct_length(char *p) {
  if (startswith(p, "==") || startswith(p, "!=") ||
      startswith(p, ">=") || startswith(p, "<=")) {
    return 2;
  }
  return ispunct(*p) ? 1 : 0;
}

static Token *tokenize(void) {
  char *p = current_input;   
  Token head = {};
  Token *cur = &head;

  while (*p) {

    // Skip whitespace characters
    if (isspace(*p)) {
      p++;
      continue;
    }

    // Numeric literal
    if (isdigit(*p)) {
      cur = cur->next = new_token(T_NUM, p, p);
      char *q = p;
      cur->val = strtoul(p, &p, 10);
      cur->len = p - q;
      continue;
    }
    // Punctuator
    int punct_len = get_punct_length(p);
    if (punct_len) {
      cur = cur->next = new_token(T_PUNCT, p, p + punct_len);
      p += punct_len;
      continue;
    }
    error_at(p, "invalid token");
  }
  cur = cur->next = new_token(T_EOF, p, p);
  return head.next;
}


// Parser

typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_EQ, // --
  ND_NE, // !=
  ND_LT, // < or >
  ND_LE, // <= or >=
  ND_NEG, // unary -
  ND_NUM, // Integer
} NodeType;

// AST Node type
typedef struct Node {
  NodeType type; // Type of Node
  struct Node *left; // left-side of AST
  struct Node *right; // right-side of AST
  int val; // Only used if type == ND_NUM
} Node;

static Node *expr(Token **rest, Token *token);
static Node *equality(Token **rest, Token *token);
static Node *relational(Token **rest, Token *token);
static Node *add(Token **rest, Token *token);
static Node *mul(Token **rest, Token *token);
static Node *unary(Token **rest, Token *token);
static Node *primary(Token **rest, Token *token);

static Node *new_node(NodeType type) {
  Node* node = calloc(1, sizeof(Node));
  node->type = type;
  return node;
}

static Node *new_binary(NodeType type, Node *left, Node *right) {
  Node *node = new_node(type);
  node->left = left;
  node->right = right;
  return node;
}

static Node *new_num(int val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

static Node *new_unary(NodeType type, Node *expr) {
  Node *node = new_node(type);
  node->left = expr;
  return node;
}



// expr = equality
static Node *expr(Token **rest, Token *token) {
  return equality(rest, token);
}

// equality = relational ("==" relational | "!=" relational)* 
static Node *equality(Token **rest, Token *token) {
  Node *node = relational(&token, token);
  for (;;) {
    if (equal(token, "==")) {
      node = new_binary(ND_EQ, node, relational(&token, token->next));
      continue;
    }

    if (equal(token, "!=")) {
      node = new_binary(ND_NE, node, relational(&token, token->next));
      continue;
    }

    *rest = token;
    return node;
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(Token **rest, Token *token) {
  Node *node = add(&token, token);

  for (;;) {
    if (equal(token, "<")) {
      node = new_binary(ND_LT, node, add(&token, token->next));
      continue;
    }

    if (equal(token, "<=")) {
      node = new_binary(ND_LE, node, add(&token, token->next));
      continue;
    }

    if (equal(token, ">")) {
      node = new_binary(ND_LT, add(&token, token->next), node);
      continue;
    }

    if (equal(token, ">=")) {
      node = new_binary(ND_LE, add(&token, token->next), node);
      continue;
    }

    *rest = token;
    return node;
  }
}

// add = mul ("+" mul | "-" mul)*
static Node *add(Token **rest, Token *token) {
  Node *node = mul(&token, token);
  for (;;) {
    if (equal(token, "+")) {
      node = new_binary(ND_ADD, node, mul(&token, token->next));
      continue;
    }

    if (equal(token, "-")) {
      node = new_binary(ND_SUB, node, mul(&token, token->next));
      continue;
    }
    *rest = token;
    return node;
  }
}

// mul = unary ("*" unary | "/" unary)*
static Node *mul(Token **rest, Token *token) {
  Node *node = unary(&token, token); 

  for (;;) {
    if (equal(token, "*")) {
      node = new_binary(ND_MUL, node, unary(&token, token->next));
      continue;
    }

    if (equal(token, "/")) {
      node = new_binary(ND_DIV, node, unary(&token, token->next));
      continue;
    }

    *rest = token;
    return node;
  }
}

// unary = ("+" | "-") unary 
//       | primary
static Node *unary(Token **rest, Token *token) {
  if (equal(token, "+")) return unary(rest, token->next);
  if (equal(token, "-")) return new_unary(ND_NEG, unary(rest, token->next));
  return primary(rest, token);
}

// primary = "(" expr ")" | num
static Node *primary(Token **rest, Token* token) {
  // Skip parenthesis(idk how to spell it)
  if (equal(token, "(")) {
    Node *node = expr(&token, token->next);
    *rest = skip(token, ")");
    return node;
  }

  if (token->type == T_NUM) {
    Node *node = new_num(token->val);
    *rest = token->next;
    return node;
  } 
  error_tok(token, "expected an expression");
  return NULL; // It will exit before here
}

// Code generator
static int depth;

static void push() {
  printf("  push %%rax\n");
  depth++;
}

static void pop(char *arg) {
  printf("  pop %s\n", arg);
  depth--;
}

static void gen_expr(Node *node) {
  switch (node->type) {
    case ND_NUM:
      printf("  mov $%d, %%rax\n", node->val);
      return;
    case ND_NEG:
      gen_expr(node->left);
      printf("  neg %%rax\n");
      return;
    default:
  }
  
  gen_expr(node->right);
  push();
  gen_expr(node->left);
  pop("%rdi");

  switch(node->type) {
    case ND_ADD:
      printf("  add %%rdi, %%rax\n");
      return;
    case ND_SUB:
      printf("  sub %%rdi, %%rax\n");
      return;
    case ND_MUL:
      printf("  imul %%rdi, %%rax\n");
      return;
    case ND_DIV:
      printf("  cqo\n");
      printf("  idiv %%rdi\n");
      return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
      printf("  cmp %%rdi, %%rax\n");

      if (node->type == ND_EQ) 
        printf("  sete %%al\n");
      else if (node->type == ND_NE) 
        printf("  setne %%al\n");
      else if (node->type == ND_LT) 
        printf("  setl %%al\n");
      else if (node->type == ND_LE)
        printf("  setle %%al\n");
      
      printf("  movzb %%al, %%rax\n");
      return;
    default:
  }

  error("invalid expression");
}



int main(int argc, char **argv) {
  if (argc != 2) error("%s: invalid number of arguments", argv[0]);
  current_input = argv[1];
  Token *token = tokenize();
  Node *node = expr(&token, token);

  if (token->type != T_EOF) {
    error_tok(token, "extra token");
  }

  printf("  .globl main\n");
  printf("main:\n");

  gen_expr(node);
  printf("  ret\n");
  assert(depth == 0);

  return 0;
}
