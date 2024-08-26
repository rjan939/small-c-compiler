#include "token.h"

Type *ty_char = &(Type) {TY_CHAR, 1, 1};
Type *ty_int = &(Type) {TY_INT, 4, 4};
Type *ty_long = &(Type) {TY_LONG, 8, 8};

static Type *new_type(TypeKind kind, int size, int align) {
  Type *type = calloc(1, sizeof(Type));
  if (type == NULL) 
    error("not enough memory in system to allocate for type");
  type->kind = kind;
  type->size = size;
  type->align = align;
  return type;
}

bool is_integer(Type *type) {
  TypeKind k = type->kind;
  return k == TY_CHAR || k == TY_INT || k == TY_LONG;
}

Type *copy_type(Type *type) {
  Type *ret = calloc(1, sizeof(Type));
  if (ret == NULL)
    error("Not enough memory in system to create type");
  *ret = *type;
  return ret;
}

Type *pointer_to(Type *base) {
  Type *type = new_type(TY_PTR, 8, 8);
  type->base = base;
  return type;
}

Type *func_type(Type *return_type) {
  Type *type = calloc(1, sizeof(Type));
  if (type == NULL)
    error("Not enough memory in system");
  type->kind = TY_FUNC;
  type->return_type = return_type;
}

Type *array_of(Type *base, int len) {
  Type *type = new_type(TY_ARRAY, base->size * len, base->align);
  type->base = base;
  type->array_len = len;
  return type;
}

void add_type(Node *node) {
  if (!node || node->type)
    return;
  
  add_type(node->left);
  add_type(node->right);
  add_type(node->cond);
  add_type(node->then);
  add_type(node->els);
  add_type(node->init);
  add_type(node->inc);

  for (Node *n = node->body; n; n = n->next)
    add_type(n);
  for (Node *n = node->args; n; n = n->next)
    add_type(n);
  switch (node->node_type) {
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
    case ND_NEG:
      node->type = node->left->type;
      return;
    case ND_ASSIGN:
      if (node->left->type->kind == TY_ARRAY)
        error_tok(node->left->token, "not a local variable");
      node->type = node->left->type;
      return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
    case ND_NUM:
    case ND_FUNCALL:
      node->type = ty_long;
      return;
    case ND_VAR:
      node->type = node->var->type;
      return;
    case ND_COMMA:
      node->type = node->right->type;
      return;
    case ND_MEMBER:
      node->type = node->member->type;
      return;
    case ND_ADDRESS:
      if (node->left->type->kind == TY_ARRAY)
        node->type = pointer_to(node->left->type->base);
      else
        node->type = pointer_to(node->left->type);
      return;
    case ND_DEREF:
      if (!node->left->type->base)
        error_tok(node->token, "invalid pointer dereference");
      node->type = node->left->type->base;
      return;
    case ND_STATEMENT_EXPRESSION:
      if (node->body) {
        Node *statement = node->body;
        while (statement->next)
          statement = statement->next;
        if (statement->node_type == ND_STATEMENT) {
          node->type = statement->left->type;
          return;
        }
      }
      error_tok(node->token, "statement expression returning void is not supported");
      return;
  }
}