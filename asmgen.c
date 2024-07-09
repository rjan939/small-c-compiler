#include "token.h"

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

void generate_asm(Node *node) {
  printf("  .globl main\n");
  printf("main:\n");

  gen_expr(node);

  printf("  ret\n");

  assert(depth==0);
}
