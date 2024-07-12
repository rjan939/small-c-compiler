#include "token.h"


static Node *expr(Token **rest, Token *token);
static Node *expr_statement(Token **rest, Token *token);
static Node *assign(Token **rest, Token *token);
static Node *equality(Token **rest, Token *token);
static Node *relational(Token **rest, Token *token);
static Node *add(Token **rest, Token *token);
static Node *mul(Token **rest, Token *token);
static Node *unary(Token **rest, Token *token);
static Node *primary(Token **rest, Token *token);

static Node *new_node(NodeType type) {
  Node* node = calloc(1, sizeof(Node));
  if (node == NULL) {
    error("not enough memory in system");
  }
  node->type = type;
  return node;
}

void free_node(Node *node) {
  if (!node) return;
  free_node(node->left);
  free_node(node->right);
  free(node);
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

static Node *new_var_node(char name) {
  Node *node = new_node(ND_VAR);
  node->name = name;
  // Calculate absolute address of given node
  node->offset = (node->name - 'a' + 1) * 8;
  return node;
}

static Node *new_unary(NodeType type, Node *expr) {
  Node *node = new_node(type);
  node->left = expr;
  return node;
}

// stmt = expr->stmt
static Node *statement(Token **rest, Token *token) {
  return expr_statement(rest, token);
}

// expr-statement = expr ";"
static Node *expr_statement(Token **rest, Token *token) {
  Node *node = new_unary(ND_STATEMENT, expr(&token, token));
  *rest = skip(token, ";");
  return node;
}

// expr = assign
static Node *expr(Token **rest, Token *token) {
  return assign(rest, token);
}

// assign = equality ("=" assign)?
static Node *assign(Token **rest, Token *token) {
  Node *node = equality(&token, token);
  if (equal(token, "=")) {
    node = new_binary(ND_ASSIGN, node, assign(&token, token->next));
  }
  *rest = token;
  return node;
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

// primary = "(" expr ")" | ident | num
static Node *primary(Token **rest, Token* token) {
  // Skip parenthesis(idk how to spell it)
  if (equal(token, "(")) {
    Node *node = expr(&token, token->next);
    *rest = skip(token, ")");
    return node;
  }

  if (token->type == T_IDENT) {
    Node *node = new_var_node(*token->loc);
    *rest = token->next;
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

// program = statement*
Node *parse(Token *token) {
  Node head = {};
  Node *curr = &head;
  while (token->type != T_EOF) {
    curr = curr->next = statement(&token, token);
  }
  return head.next;
}
