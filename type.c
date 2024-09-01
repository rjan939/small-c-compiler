#include "token.h"

Type *ty_void = &(Type) {TY_VOID, 1, 1};
Type *ty_bool = &(Type) {TY_BOOL, 1, 1};

Type *ty_char = &(Type) {TY_CHAR, 1, 1};
Type *ty_short = &(Type) {TY_SHORT, 2, 2};
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
  return k == TY_BOOL || k == TY_CHAR || k == TY_SHORT || 
        k == TY_INT || k == TY_LONG || k == TY_ENUM;
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

Type *enum_type(void) {
  return new_type(TY_ENUM, 4, 4);
}

static Type *get_common_type(Type *type1, Type *type2) {
  if (type1->base)
    return pointer_to(type1->base);
  if (type1->size == 8 || type2->size == 8)
    return ty_long;
  return ty_int;
}

// If the type of one operand is larger than the other(ex. long v int), the smaller operand will be promoted to match with the other.
// This is called usual arithmetic conversion
static void usual_arithmetic_conversion(Node **left, Node **right) {
  Type *type = get_common_type((*left)->type, (*right)->type);
  *left = new_cast(*left, type);
  *right = new_cast(*right, type);
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
    case ND_NUM:
      node->type = (node->val == (int)node->val) ? ty_int : ty_long;
      return;
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
      usual_arithmetic_conversion(&node->left, &node->right);
      node->type = node->left->type;
      return;
    case ND_NEG:
      Type *type = get_common_type(ty_int, node->left->type);
      node->left = new_cast(node->left, type);
      node->type = type;
      return;
    case ND_ASSIGN:
      if (node->left->type->kind == TY_ARRAY)
        error_tok(node->left->token, "not a local variable");
      if (node->left->type->kind != TY_STRUCT)
        node->right = new_cast(node->right, node->left->type);
      node->type = node->left->type;
      return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
      usual_arithmetic_conversion(&node->left, &node->right);
      node->type = ty_int;
      return;
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
      if (node->left->type->base->kind == TY_VOID)
        error_tok(node->token, "dereferencing a void pointer");
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