// This file is a recursive descent parser for C
#include "token.h"

// Local variables in the parser
LVar *locals;

static Node *compound_statement(Token **rest, Token *token);
static Node *statement(Token **rest, Token *token);
static Node *expr(Token **rest, Token *token);
static Node *expr_statement(Token **rest, Token *token);
static Node *assign(Token **rest, Token *token);
static Node *equality(Token **rest, Token *token);
static Node *relational(Token **rest, Token *token);
static Node *add(Token **rest, Token *token);
static Node *mul(Token **rest, Token *token);
static Node *unary(Token **rest, Token *token);
static Node *primary(Token **rest, Token *token);

// Find a local variable based on name
static LVar *find_var(Token *token) {
  for (LVar *var = locals; var; var = var->next)
    if (strlen(var->name) == token->len && !strncmp(token->loc, var->name, token->len))
      return var;
  return NULL;
}

static Node *new_node(NodeType type) {
  Node* node = calloc(1, sizeof(Node));
  if (node == NULL) {
    error("not enough memory in system");
  }
  node->nodeType = type;
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

static Node *new_var_node(LVar *var) {
  Node *node = new_node(ND_VAR);
  node->var = var;
  return node;
}

static LVar *new_lvar(char *name) {
  LVar *var = calloc(1, sizeof(LVar));
  if (var == NULL) {
    error("not enough memory in system");
  }

  var->name = name;
  var->next = locals;
  var->len = strlen(name);
  locals = var;
  return var;
}

void free_lvar(LVar *locals) {
  if (!locals) return;
  free_lvar(locals->next);
  free(locals);
}


static Node *new_unary(NodeType type, Node *expr) {
  Node *node = new_node(type);
  node->left = expr;
  return node;
}

// stmt = "return" expr ";" 
//        | "if" "(" expr ")" stmt ("else" stmt)?
//        | "for" "(" expr-stmt expr? ";" expr? ")" statement
//        | "while" "(" expr ")" stmt
//        | "{" compound-stmt
//        | expr->stmt
static Node *statement(Token **rest, Token *token) {
  if (equal(token, "return")) {
    Node *node = new_unary(ND_RETURN, expr(&token, token->next));
    *rest = skip(token, ";");
    return node;
  }

  if (equal(token, "if")) {
    Node *node = new_node(ND_IF);
    token = skip(token->next, "(");
    node->cond = expr(&token, token);
    token = skip(token, ")");
    node->then = statement(&token, token);

    if (equal(token, "else"))
      node->els = statement(&token, token->next);
    *rest = token;
    return node;
  }

  if (equal(token, "for")) {
    Node *node = new_node(ND_FOR);
    token = skip(token->next, "(");

    node->init = expr_statement(&token, token);

    if (!equal(token, ";"))
      node->cond = expr(&token, token);
    token = skip(token, ";");

    if (!equal(token, ")"))
      node->inc = expr(&token, token);
    token = skip(token, ")");

    node->then = statement(rest, token);
    return node;
  }

  if (equal(token, "while")) {
    Node *node = new_node(ND_FOR);
    token = skip(token->next, "(");
    node->cond = expr(&token, token);
    token = skip(token, ")");
    node->then = statement(rest, token);
    return node;
  }

  if (equal(token, "{"))
    return compound_statement(rest, token->next);

  return expr_statement(rest, token);
}

// compound-stmt = stmt* "}"
static Node *compound_statement(Token **rest, Token* token) {
  Node head = {};
  Node *cur = &head;
  while (!equal(token, "}")) {
    cur = cur->next = statement(&token, token);
    add_type(cur);
  }

  Node *node = new_node(ND_BLOCK);
  node->body = head.next;
  *rest = token->next;
  return node;
}

// expr-statement = expr? ";"
static Node *expr_statement(Token **rest, Token *token) {
  if (equal(token, ";")) {
    *rest = token->next;
    return new_node(ND_NULL_STATEMENT);
  }
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

// In C, '+' operator is overloaded to do pointer arithmetic
// if p is a pointer, p + n adds not n but sizeof(*p)*n to the value of p,
// so that p + n points to the location n elements ahead of p
// scale integer value before adding to a pointer value
// which is what this function does
static Node *new_add(Node *left, Node *right, Token *token) {
  add_type(left);
  add_type(right);

  // num + num
  if (is_integer(left->type) && is_integer(right->type))
    return new_binary(ND_ADD, left, right);
  
  if (left->type->base && right->type->base)
    error_tok(token, "invalid operands");
  
  // num + ptr
  if (!left->type->base && right->type->base) {
    Node *tmp = left;
    left = right;
    right = tmp;
  }

  // ptr + num
  right = new_binary(ND_MUL, right, new_num(8));
  return new_binary(ND_ADD, left, right);
}

static Node *new_sub(Node *left, Node *right, Token *token) {
  add_type(left);
  add_type(right);

  // num - num
  if (is_integer(left->type) && is_integer(right->type))
    return new_binary(ND_SUB, left, right);
  
  // ptr - num
  if (left->type->base && is_integer(right->type)) {
    right = new_binary(ND_MUL, right, new_num(8));
    add_type(right);
    Node *node = new_binary(ND_SUB, left, right);
    node->type = left->type;
    return node;
  }
  
  // ptr - ptr, which returns how many elements are between the two
  if (left->type->base && right->type->base) {
    Node *node = new_binary(ND_SUB, left, right);
    node->type = ty_int;
    return new_binary(ND_DIV, node, new_num(8));
  }

  error_tok(token, "invalid operands");
}

// add = mul ("+" mul | "-" mul)*
static Node *add(Token **rest, Token *token) {
  Node *node = mul(&token, token);
  for (;;) {
    Token *start = token;
    if (equal(token, "+")) {
      node = new_add(node, mul(&token, token->next), start);
      continue;
    }

    if (equal(token, "-")) {
      node = new_sub(node, mul(&token, token->next), start);
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

// unary = ("+" | "-" | "*" | "&") unary 
//       | primary
static Node *unary(Token **rest, Token *token) {
  if (equal(token, "+")) 
    return unary(rest, token->next);
  if (equal(token, "-")) 
    return new_unary(ND_NEG, unary(rest, token->next));
  if (equal(token, "&")) 
    return new_unary(ND_ADDRESS, unary(rest, token->next));
  if (equal(token, "*")) 
    return new_unary(ND_DEREF, unary(rest, token->next));
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
    // Verify that there is not a variable already created with this name
    LVar *var = find_var(token);
    if (!var) 
      var = new_lvar(strndup(token->loc, token->len));
    *rest = token->next;
    return new_var_node(var);
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
Function *parse(Token *token) {
  token = skip(token, "{");
  
  Function *program = calloc(1, sizeof(Function));
  if (program == NULL) 
    error("not enough memory in system");
  program->body = compound_statement(&token, token);
  program->locals = locals;
  return program;
}
