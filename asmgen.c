#include "token.h"

// Code generator
static int depth;

static int count(void) {
  static int i = 1;
  return i++; 
}

static void push(void) {
  printf("  push %%rax\n");
  depth++;
}

static void pop(char *arg) {
  printf("  pop %s\n", arg);
  depth--;
}

// Round up 'n' to the nearest multiple of 'align'. For example,
// align_to(5, 8) returns 8 and align_to(11, 8) returns 16
static int align_to(int n, int align) {
  return (n + align - 1) / align * align;
}

static void gen_address(Node *node) {
  if (node->type == ND_VAR) {
    printf("  lea %d(%%rbp), %%rax\n", node->var->offset);
    return;
  }


  error("not a local variable");
}


// Generate assembly code to handle the logic of given node 
static void gen_expr(Node *node) {
  switch (node->type) {
    case ND_NUM:
      printf("  mov $%d, %%rax\n", node->val);
      return;
    case ND_NEG:
      gen_expr(node->left);
      printf("  neg %%rax\n");
      return;
    case ND_VAR:
      gen_address(node);
      printf("  mov (%%rax), %%rax\n");
      return;
    case ND_ASSIGN:
      gen_address(node->left);
      push();
      gen_expr(node->right);
      pop("%rdi");
      printf("  mov %%rax, (%%rdi)\n");
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

static void gen_statement(Node *node) {
  switch (node->type) {
    case ND_IF:
      int c = count();
      gen_expr(node->cond);
      printf("  cmp $0, %%rax\n");
      printf("  je  .L.else.%d\n", c);
      gen_statement(node->then);
      printf("  jmp .L.end.%d\n", c);
      printf(".L.else.%d:\n", c);
      if (node->els)
        gen_statement(node->els);
      printf(".L.end.%d:\n", c);
      return;
    case ND_NULL_STATEMENT:
      return;
    case ND_BLOCK:
      for (Node *n = node->body; n; n = n->next)
        gen_statement(n);
      return;
    case ND_RETURN:
      gen_expr(node->left);
      printf(" jmp .L.return\n");
      return;
    case ND_STATEMENT:
      gen_expr(node->left);
      return;
    default:
  }

  error("invalid statement");
}

// Source: https://stackoverflow.com/questions/70778878/how-do-programs-know-how-much-space-to-allocate-for-local-variables-on-the-stack
// Assigns offsets to local variables for memory allocation
static void assign_lvar_offsets(Function *program) {
  int offset = 0;
  for (LVar *var = program->locals; var; var = var->next) {
    offset += 8;
    var->offset = -offset;
  }
  program->stack_size = align_to(offset, 16);
}

void gen_asm(Function *program) {
  assign_lvar_offsets(program);
  printf("  .globl main\n");
  printf("main:\n");

  // Setting up stack frame
  // %rbp is the base pointer register in this implementation
  printf("  push %%rbp\n"); // Save caller's base pointer
  printf("  mov %%rsp, %%rbp\n"); // Set the base pointer to the current stack pointer
  printf("  sub $%d, %%rsp\n", program->stack_size); // Allocate space 


  gen_statement(program->body);
  assert(depth == 0);

  printf(".L.return:\n");

  // Tear down stack frame
  printf("  mov %%rbp, %%rsp\n"); // Reset stack pointer
  printf("  pop %%rbp\n"); // Restore caller's base pointer
  printf("  ret\n");
}
