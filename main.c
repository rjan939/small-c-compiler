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

static bool equal(Token *token, char *op) {
    return memcmp(token->loc, op, token->len) == 0 && op[token->len] == '\0';
}

static Token *skip(Token *token, char *s) {
    if (!equal(token, s)) error("expected '%s'", s);
    return token->next;
}

int get_number(Token *token) {
    if (token->type != T_NUM) error("expected a number");
    return token->val;
}

static Token *new_token(TokenType type, char *start, char *end) {
    Token *token = calloc(1, sizeof(Token));
    token->type = type;
    token->loc = start;
    token->len = end - start;
    return token;
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
        if (ispunct(*p)) {
            cur = cur->next = new_token(T_PUNCT, p, p + 1);
            p++;
            continue;
        }

        error("invalid token");
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
static Node *mul(Token **rest, Token *token);
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



// expr = mul ("+" mul | "-" mul)*
static Node *expr(Token **rest, Token *token) {
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

// mul = primary ("*" primary | "/" primary)*
static Node *mul(Token **rest, Token *token) {
  Node *node = primary(&token, token);
  for (;;) {
    if (equal(token, "*")) {
      node = new_binary(ND_MUL, node, primary(&token, token->next));
      continue;
    }

    if (equal(token, "/")) {
      node = new_binary(ND_DIV, node, primary(&token, token->next));
      continue;
    }

    *rest = token;
    return node;
  }
}

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
  error("expected an expression");
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
  if (node->type == ND_NUM) {
    printf("  mov $%d, %%rax\n", node->val);
    return;
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
      error("extra token");
    }

    printf("  .globl main\n");
    printf("main:\n");

    // First token has to be a number
    /*printf("  mov $%d, %%rax\n", get_number(token));
    token = token->next;

    while (token->type != T_EOF) {
        if (equal(token, "+")) {
            printf("  add $%d, %%rax\n", get_number(token->next));
            token = token->next->next;
            continue;
      
        }

        token = skip(token, "-");
        printf("  sub $%d, %%rax\n", get_number(token));
        token = token->next;
    }*/ 
    gen_expr(node);
    printf("  ret\n");
    assert(depth == 0);
    return 0;
}
