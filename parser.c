// This file is a recursive descent parser for C
#include "token.h"

// Local variables in the parser
LVar *locals;

static Type *declaration_specifier(Token **rest, Token *token);
static Type *declarator(Token **rest, Token *token, Type *type);
static Node *declaration(Token **rest, Token *token);
static Node *compound_statement(Token **rest, Token *token);
static Node *statement(Token **rest, Token *token);
static Node *expr_statement(Token **rest, Token *token);
static Node *expr(Token **rest, Token *token);
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

static Node *new_node(NodeType type, Token *token) {
  Node* node = calloc(1, sizeof(Node));
  if (node == NULL) {
    error("not enough memory in system");
  }
  node->nodeType = type;
  node->token = token;
  return node;
}

void free_node(Node *node) {
  if (!node) return;
  free_node(node->left);
  free(node->type);
  free_node(node->right);
  free(node->type);
  free(node);
}


static Node *new_binary(NodeType type, Node *left, Node *right, Token *token) {
  Node *node = new_node(type, token);
  node->left = left;
  node->right = right;
  return node;
}

static Node *new_num(int val, Token *token) {
  Node *node = new_node(ND_NUM, token);
  node->val = val;
  return node;
}

static Node *new_var_node(LVar *var, Token *token) {
  Node *node = new_node(ND_VAR, token);
  node->var = var;
  return node;
}

static LVar *new_lvar(char *name, Type *type) {
  LVar *var = calloc(1, sizeof(LVar));
  if (var == NULL) {
    error("not enough memory in system");
  }

  var->name = name;
  var->type = type;
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

static Node *new_unary(NodeType type, Node *expr, Token *token) {
  Node *node = new_node(type, token);
  node->left = expr;
  return node;
}

static char *get_ident(Token *token) {
  if (token->type != T_IDENT)
    error_tok(token, "expected an identifier");
  return strndup(token->loc, token->len);
}

static int get_number(Token *token) {
  if (token->type != T_NUM)
    error_tok(token, "expected a number");
  return token->val;
}

// declaration-specifier = "int"
static Type *declaration_specifier(Token **rest, Token *token) {
  *rest = skip(token, "int");
  return ty_int;
}

// func-params = (param ("," param)*)? ")"
// param       = declaration-specifier declarator
static Type *func_params(Token **rest, Token *token, Type *type) {
  Type head = {};
  Type *cur = &head;

  while (!equal(token, ")")) {
    if (cur != &head)
      token = skip(token, ",");
    Type *basetype = declaration_specifier(&token, token);
    Type *type = declarator(&token, token, basetype);
    cur = cur->next = copy_type(type);
  }
  
  type = func_type(type);
  type->params = head.next;
  *rest = token->next;
  return type;
}

// type-suffix = "(" func-params
//             | "[" num "]"
//             | ε
static Type *type_suffix(Token **rest, Token *token, Type *type) {
  if (equal(token, "(")) 
    return func_params(rest, token->next, type);
  
  if (equal(token, "[")) {
    int size = get_number(token->next);
    *rest = skip(token->next->next, "]");
    return array_of(type, size);
  }

  *rest = token;
  return type;
}

// declarator = "*"* ident type-suffix
static Type *declarator(Token **rest, Token *token, Type *type) {
  while (consume(&token, token, "*"))
    type = pointer_to(type);
  
  if (token->type != T_IDENT)
    error_tok(token, "expected a variable name");
  
  type = type_suffix(rest, token->next, type);
  type->name = token;
  return type;
}

// declaration = declaration_specifier (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
static Node *declaration(Token **rest, Token *token) {
  Type *basetype = declaration_specifier(&token, token);

  Node head = {};
  Node *cur = &head;
  int i = 0;

  while (!equal(token, ";")) {
    if (i++ > 0)
      token = skip(token, ",");
    
    Type *type = declarator(&token, token, basetype);
    LVar *var = new_lvar(get_ident(type->name), type);
    
    if (!equal(token, "="))
      continue;
    
    Node *left = new_var_node(var, type->name);
    Node *right = assign(&token, token->next);
    Node *node = new_binary(ND_ASSIGN, left, right, token);
    cur = cur->next = new_unary(ND_STATEMENT, node, token);
  }

  Node *node = new_node(ND_BLOCK, token);
  node->body = head.next;
  *rest = token->next;
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
    Node *node = new_node(ND_RETURN, token);
    node->left = expr(&token, token->next);
    *rest = skip(token, ";");
    return node;
  }

  if (equal(token, "if")) {
    Node *node = new_node(ND_IF, token);
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
    Node *node = new_node(ND_FOR, token);
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
    Node *node = new_node(ND_FOR, token);
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

// compound-stmt = (declaration | stmt)* "}"
static Node *compound_statement(Token **rest, Token* token) {
  Node *node = new_node(ND_BLOCK, token);
  Node head = {};
  Node *cur = &head;
  while (!equal(token, "}")) {
    if (equal(token, "int"))
      cur = cur->next = declaration(&token, token);
    else
      cur = cur->next = statement(&token, token);
    add_type(cur);
  }

  node->body = head.next;
  *rest = token->next;
  return node;
}

// expr-statement = expr? ";"
static Node *expr_statement(Token **rest, Token *token) {
  if (equal(token, ";")) {
    *rest = token->next;
    return new_node(ND_NULL_STATEMENT, token);
  }
  Node *node = new_node(ND_STATEMENT, token);
  node->left = expr(&token, token);
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
    node = new_binary(ND_ASSIGN, node, assign(&token, token->next), token);
  }
  *rest = token;
  return node;
}

// equality = relational ("==" relational | "!=" relational)* 
static Node *equality(Token **rest, Token *token) {
  Node *node = relational(&token, token);
  for (;;) {
    Token *start = token;
    if (equal(token, "==")) {
      node = new_binary(ND_EQ, node, relational(&token, token->next), start);
      continue;
    }

    if (equal(token, "!=")) {
      node = new_binary(ND_NE, node, relational(&token, token->next), start);
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
    Token *start = token;
    if (equal(token, "<")) {
      node = new_binary(ND_LT, node, add(&token, token->next), start);
      continue;
    }

    if (equal(token, "<=")) {
      node = new_binary(ND_LE, node, add(&token, token->next), start);
      continue;
    }

    if (equal(token, ">")) {
      node = new_binary(ND_LT, add(&token, token->next), node, start);
      continue;
    }

    if (equal(token, ">=")) {
      node = new_binary(ND_LE, add(&token, token->next), node, start);
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
    return new_binary(ND_ADD, left, right, token);
  
  if (left->type->base && right->type->base)
    error_tok(token, "invalid operands");
  
  // num + ptr
  if (!left->type->base && right->type->base) {
    Node *tmp = left;
    left = right;
    right = tmp;
  }

  // ptr + num
  right = new_binary(ND_MUL, right, new_num(left->type->base->size, token), token);
  return new_binary(ND_ADD, left, right, token);
}

static Node *new_sub(Node *left, Node *right, Token *token) {
  add_type(left);
  add_type(right);

  // num - num
  if (is_integer(left->type) && is_integer(right->type))
    return new_binary(ND_SUB, left, right, token);
  
  // ptr - num
  if (left->type->base && is_integer(right->type)) {
    right = new_binary(ND_MUL, right, new_num(left->type->base->size, token), token);
    add_type(right);
    Node *node = new_binary(ND_SUB, left, right, token);
    node->type = left->type;
    return node;
  }
  
  // ptr - ptr, which returns how many elements are between the two
  if (left->type->base && right->type->base) {
    Node *node = new_binary(ND_SUB, left, right, token);
    node->type = ty_int;
    return new_binary(ND_DIV, node, new_num(left->type->base->size, token), token);
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
    Token *start = token;

    if (equal(token, "*")) {
      node = new_binary(ND_MUL, node, unary(&token, token->next), start);
      continue;
    }

    if (equal(token, "/")) {
      node = new_binary(ND_DIV, node, unary(&token, token->next), start);
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
    return new_unary(ND_NEG, unary(rest, token->next), token);
  if (equal(token, "&")) 
    return new_unary(ND_ADDRESS, unary(rest, token->next), token);
  if (equal(token, "*")) 
    return new_unary(ND_DEREF, unary(rest, token->next), token);
  return primary(rest, token);
}

// funcall = ident "(" (assign ("," assign)*)? ")"
static Node *funcall(Token **rest, Token *token) {
  Token *start = token;
  token = token->next->next;

  Node head = {};
  Node *cur = &head;

  while (!equal(token, ")")) {
    if (cur != &head)
      token = skip(token, ",");
    cur = cur->next = assign(&token, token);
  }

  *rest = skip(token, ")");

  Node *node = new_node(ND_FUNCALL, start);
  node->funcname = strndup(start->loc, start->len);
  node->args = head.next;
  return node;
}

// primary = "(" expr ")" | funcall | num
static Node *primary(Token **rest, Token* token) {
  // Skip parenthesis(idk how to spell it)
  if (equal(token, "(")) {
    Node *node = expr(&token, token->next);
    *rest = skip(token, ")");
    return node;
  }

  if (token->type == T_IDENT) {
    // Function call
    if (equal(token->next, "(")) {
      return funcall(rest, token);
    }

    // Verify that there is not a variable already created with this name
    LVar *var = find_var(token);
    if (!var) 
      error_tok(token, "undefined variable");
    *rest = token->next;
    return new_var_node(var, token);
  }

  if (token->type == T_NUM) {
    Node *node = new_num(token->val, token);
    *rest = token->next;
    return node;
  } 
  error_tok(token, "expected an expression");
  return NULL; // It will exit before here
}

static void create_param_lvars(Type *param) {
  if (param) {
    create_param_lvars(param->next);
    new_lvar(get_ident(param->name), param);
  }
}

static Function *function(Token **rest, Token *token) {
  Type *type = declaration_specifier(&token, token);
  type = declarator(&token, token, type);

  locals = NULL;

  Function *func = calloc(1, sizeof(Function));
  if (func == NULL)
    error("Not enough memory in system for function");
  func->name = get_ident(type->name);
  create_param_lvars(type->params);
  func->params = locals;

  token = skip(token, "{");
  func->body = compound_statement(rest, token);
  func->locals = locals;
  return func;
}



// program = function-definition*
Function *parse(Token *token) {
  Function head = {};
  Function *cur = &head;
  
  while (token->type != T_EOF)
    cur = cur->next = function(&token, token);
  return head.next;
}
