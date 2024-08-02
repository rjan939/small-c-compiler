#include "token.h"

Type *ty_int = &(Type) {TY_INT};

bool is_integer(Type *type) {
  return type->kind == TY_INT;
}

Type *pointer_to(Type *base) {
  Type *type = calloc(1, sizeof(Type));
  if (type == NULL)
    error("Not enough memory in system");
  type->kind = TY_PTR;
  type->base = base;
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
  
  switch (node->nodeType) {
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
    case ND_NEG:
    case ND_ASSIGN:
      node->type = node->left->type;
      return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
    case ND_VAR:
    case ND_NUM:
      node->type = ty_int;
      return;
    case ND_ADDRESS:
      node->type = pointer_to(node->left->type);
      return;
    case ND_DEREF:
      if (node->left->type->kind == TY_PTR)
        node->type = node->left->type->base;
      else
        node->type = ty_int;
      return;
  }
}