#include "token.h"

// Code generator
static int depth;
static char *argreg[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
static Function *current_func;

static void gen_expr(Node *node);

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
  switch (node->nodeType) {
    case ND_VAR: 
      printf("  lea %d(%%rbp), %%rax\n", node->var->offset);
      return;
    case ND_DEREF:
      gen_expr(node->left);
      return;
  }


  error_tok(node->token, "not a local variable");
}

// Load a value from where %rax is pointing to
static void load(Type *type) {
  if (type->kind == TY_ARRAY) {
    // If it is an array, dont load a value to the
    // register because you cant load entire arrays to regs
    // the result of an evaluation of an array becomes
    // not the array itself but the address of the array
    return;
  }

  printf("  mov (%%rax), %%rax\n");
}

// Store %rax to an address that the stack top is pointing to
static void store(void) {
  pop("%rdi");
  printf("  mov %%rax, (%%rdi)\n");
}


// Generate assembly code to handle the logic of given node 
static void gen_expr(Node *node) {
  switch (node->nodeType) {
    case ND_NUM:
      printf("  mov $%d, %%rax\n", node->val);
      return;
    case ND_NEG:
      gen_expr(node->left);
      printf("  neg %%rax\n");
      return;
    case ND_VAR:
      gen_address(node);
      load(node->type);
      return;
    case ND_DEREF:
      gen_expr(node->left);
      load(node->type);
      return;
    case ND_ADDRESS:
      gen_address(node->left);
      return;
    case ND_ASSIGN:
      gen_address(node->left);
      push();
      gen_expr(node->right);
      store();
      return;
    case ND_FUNCALL:
      int nargs = 0;
      for (Node *arg = node->args; arg; arg = arg->next) {
        gen_expr(arg);
        push();
        nargs++;
      }
      
      for (int i = nargs - 1; i >= 0; i--)
        pop(argreg[i]);
      
      printf("  mov $0, %%rax\n");
      printf("  call %s\n", node->funcname);
      return;
    default:
  }
  
  gen_expr(node->right);
  push();
  gen_expr(node->left);
  pop("%rdi");

  switch(node->nodeType) {
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

      if (node->nodeType == ND_EQ) 
        printf("  sete %%al\n");
      else if (node->nodeType == ND_NE) 
        printf("  setne %%al\n");
      else if (node->nodeType == ND_LT) 
        printf("  setl %%al\n");
      else if (node->nodeType == ND_LE)
        printf("  setle %%al\n");
      
      printf("  movzb %%al, %%rax\n");
      return;
    default:
  }

  error_tok(node->token, "invalid expression");
}

static void gen_statement(Node *node) {
  int c;
  switch (node->nodeType) {
    case ND_IF:
      c = count();
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
    case ND_FOR:
      c = count();
      if (node->init)
        gen_statement(node->init);
      printf(".L.begin.%d:\n", c);
      if (node->cond) {
        gen_expr(node->cond);
        printf("  cmp $0, %%rax\n");
        printf("  je  .L.end.%d\n", c);
      }
      gen_statement(node->then);
      if (node->inc) 
        gen_expr(node->inc);
      printf("  jmp .L.begin.%d\n", c);
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
      printf(" jmp .L.return.%s\n", current_func->name);
      return;
    case ND_STATEMENT:
      gen_expr(node->left);
      return;
    default:
  }

  error_tok(node->token, "invalid statement");
}

// Source: https://stackoverflow.com/questions/70778878/how-do-programs-know-how-much-space-to-allocate-for-local-variables-on-the-stack
// Assigns offsets to local variables for memory allocation
static void assign_lvar_offsets(Function *program) {
  int offset = 0;
  for (Function *func = program; func; func = func->next) {
    int offset = 0;
    for (LVar *var = func->locals; var; var = var->next) {
      offset += var->type->size;
      var->offset = -offset;
    }
    func->stack_size = align_to(offset, 16);
  }
}

void gen_asm(Function *program) {
  assign_lvar_offsets(program);
  
  for (Function *func = program; func; func = func->next) {
    printf("  .globl %s\n", func->name);
    printf("%s:\n", func->name);
    current_func = func;

    // Setting up stack frame
    // %rbp is the base pointer register in this implementation
    printf("  push %%rbp\n"); // Save caller's base pointer
    printf("  mov %%rsp, %%rbp\n"); // Set the base pointer to the current stack pointer
    printf("  sub $%d, %%rsp\n", func->stack_size); // Allocate space

    // Save passed-by-register args to the stack
    int i = 0;
    for (LVar *var = func->params; var; var = var->next)
      printf("  mov %s, %d(%%rbp)\n", argreg[i++], var->offset);

    // Emit code
    gen_statement(func->body);
    assert(depth == 0);

    printf(".L.return.%s:\n", func->name);

    // Tear down stack frame
    printf("  mov %%rbp, %%rsp\n"); // Reset stack pointer
    printf("  pop %%rbp\n"); // Restore caller's base pointer
    printf("  ret\n");
  }
}
