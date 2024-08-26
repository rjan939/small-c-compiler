#include "token.h"

// Code generator
static FILE *output_file;
static int depth;

static char *argreg8[] = {"%dil", "%sil", "%dl", "%cl", "%r8b", "%r9b"};
static char *argreg16[] = {"%di", "%si", "%dx", "%cx", "%r8w", "%r9w"};
static char *argreg32[] = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};
static char *argreg64[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
static Obj *current_func;

static void gen_expr(Node *node);
static void gen_statement(Node *node);

static void println(char *fmt, ...) {
  va_list argument_pointer;
  va_start(argument_pointer, fmt);
  vfprintf(output_file, fmt, argument_pointer);
  va_end(argument_pointer);
  fprintf(output_file, "\n");
}

static int count(void) {
  static int i = 1;
  return i++; 
}

static void push(void) {
  println("  push %%rax");
  depth++;
}

static void pop(char *arg) {
  println("  pop %s", arg);
  depth--;
}

// Round up 'n' to the nearest multiple of 'align'. For example,
// align_to(5, 8) returns 8 and align_to(11, 8) returns 16
int align_to(int n, int align) {
  return (n + align - 1) / align * align;
}

static void gen_address(Node *node) {
  switch (node->node_type) {
    case ND_VAR:
      if (node->var->is_local) {
        // Local variable
        println("  lea %d(%%rbp), %%rax", node->var->offset);
      } else {
        // Global variable
        println("  lea %s(%%rip), %%rax", node->var->name);
      }
      return;
    case ND_DEREF:
      gen_expr(node->left);
      return;
    case ND_COMMA:
      gen_expr(node->left);
      gen_address(node->right);
      return;
    case ND_MEMBER:
      gen_address(node->left);
      println(" add $%d, %%rax", node->member->offset);
      return;
  }


  error_tok(node->token, "not a local variable");
}

// Load a value from where %rax is pointing to
static void load(Type *type) {
  if (type->kind == TY_ARRAY || type->kind == TY_STRUCT || type->kind == TY_UNION) {
    // If it is an array, dont load a value to the
    // register because you cant load entire arrays to regs
    // the result of an evaluation of an array becomes
    // not the array itself but the address of the array
    return;
  }

  if (type->size == 1)
    println("  movsbq (%%rax), %%rax");
  else if (type->size == 2)
    println(" movswq (%%rax), %%rax");
  else if (type->size == 4)
    println(" movsxd (%%rax), %%rax");
  else
    println("  mov (%%rax), %%rax");
}

// Store %rax to an address that the stack top is pointing to
static void store(Type *type) {
  pop("%rdi");

  if (type->kind == TY_STRUCT || type->kind == TY_UNION) {
    for (int i = 0; i < type->size; i++) {
      println("  mov %d(%%rax), %%r8b", i);
      println("  mov %%r8b, %d(%%rdi)", i);
    }
    return;
  }

  if (type->size == 1)
    println("  mov %%al, (%%rdi)");
  else if (type->size == 2)
    println(" mov %%ax, (%%rdi)");
  else if (type->size == 4)
    println(" mov %%eax, (%%rdi)");
  else
    println("  mov %%rax, (%%rdi)");
}


// Generate assembly code to handle the logic of given node 
static void gen_expr(Node *node) {
  println(" .loc 1 %d", node->token->line_num);

  switch (node->node_type) {
    case ND_NUM:
      println("  mov $%ld, %%rax", node->val);
      return;
    case ND_NEG:
      gen_expr(node->left);
      println("  neg %%rax");
      return;
    case ND_VAR:
    case ND_MEMBER:
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
      store(node->type);
      return;
    case ND_STATEMENT_EXPRESSION:
      for (Node *n = node->body; n; n = n->next)
        gen_statement(n);
      return;
    case ND_COMMA:
      gen_expr(node->left);
      gen_expr(node->right);
      return;
    case ND_FUNCALL:
      int nargs = 0;
      for (Node *arg = node->args; arg; arg = arg->next) {
        gen_expr(arg);
        push();
        nargs++;
      }
      
      for (int i = nargs - 1; i >= 0; i--)
        pop(argreg64[i]);
      
      println("  mov $0, %%rax");
      println("  call %s", node->funcname);
      return;
    default:
  }
  
  gen_expr(node->right);
  push();
  gen_expr(node->left);
  pop("%rdi");

  switch(node->node_type) {
    case ND_ADD:
      println("  add %%rdi, %%rax");
      return;
    case ND_SUB:
      println("  sub %%rdi, %%rax");
      return;
    case ND_MUL:
      println("  imul %%rdi, %%rax");
      return;
    case ND_DIV:
      println("  cqo");
      println("  idiv %%rdi");
      return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
      println("  cmp %%rdi, %%rax");

      if (node->node_type == ND_EQ) 
        println("  sete %%al");
      else if (node->node_type == ND_NE) 
        println("  setne %%al");
      else if (node->node_type == ND_LT) 
        println("  setl %%al");
      else if (node->node_type == ND_LE)
        println("  setle %%al");
      
      println("  movzb %%al, %%rax");
      return;
    default:
  }

  error_tok(node->token, "invalid expression");
}

static void gen_statement(Node *node) {
  println(" .loc 1 %d", node->token->line_num);

  int c = 0;
  switch (node->node_type) {
    case ND_IF:
      c = count();
      gen_expr(node->cond);
      println("  cmp $0, %%rax");
      println("  je  .L.else.%d", c);
      gen_statement(node->then);
      println("  jmp .L.end.%d", c);
      println(".L.else.%d:", c);
      if (node->els)
        gen_statement(node->els);
      println(".L.end.%d:", c);
      return;
    case ND_FOR:
      c = count();
      if (node->init)
        gen_statement(node->init);
      println(".L.begin.%d:", c);
      if (node->cond) {
        gen_expr(node->cond);
        println("  cmp $0, %%rax");
        println("  je  .L.end.%d", c);
      }
      gen_statement(node->then);
      if (node->inc) 
        gen_expr(node->inc);
      println("  jmp .L.begin.%d", c);
      println(".L.end.%d:", c);
      return;
    case ND_NULL_STATEMENT:
      return;
    case ND_BLOCK:
      for (Node *n = node->body; n; n = n->next)
        gen_statement(n);
      return;
    case ND_RETURN:
      gen_expr(node->left);
      println(" jmp .L.return.%s", current_func->name);
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
static void assign_lvar_offsets(Obj *program) {
  int offset = 0;
  for (Obj *func = program; func; func = func->next) {
    if (!func->is_function)
      continue;

    int offset = 0;
    for (Obj *var = func->locals; var; var = var->next) {
      offset += var->type->size;
      offset = align_to(offset, var->type->align);
      var->offset = -offset;
    }
    func->stack_size = align_to(offset, 16);
  }
}

static void emit_data(Obj *program) {
  for (Obj *var = program; var; var = var->next) {
    if (var->is_function)
      continue;
    
    println("  .data");
    println("  .globl %s", var->name);
    println("%s:", var->name);
    if (var->init_data)
      for (int i = 0; i < var->type->size; i++)
        println("  .byte %d", var->init_data[i]);
    else
      println("  .zero %d", var->type->size);
  }
}

static void store_gp(int r, int offset, int sz) {
  switch (sz) {
    case 1:
      println(" mov %s, %d(%%rbp)", argreg8[r], offset);
      return;
    case 2:
      println(" mov %s, %d(%%rbp)", argreg16[r], offset);
      return;
    case 4:
      println(" mov %s, %d(%%rbp)", argreg32[r], offset);
      return;
    case 8:
      println(" mov %s, %d(%%rbp)", argreg64[r], offset);
      return;
  }
  unreachable();
}

static void emit_text(Obj *program) {
  for (Obj *func = program; func; func = func->next) {
    if (!func->is_function || !func->is_definition)
      continue;

    println("  .globl %s", func->name);
    println("  .text");
    println("%s:", func->name);
    current_func = func;

    // Setting up stack frame
    // %rbp is the base pointer register in this implementation
    println("  push %%rbp"); // Save caller's base pointer
    println("  mov %%rsp, %%rbp"); // Set the base pointer to the current stack pointer
    println("  sub $%d, %%rsp", func->stack_size); // Allocate space

    // Save passed-by-register args to the stack
    int i = 0;
    for (Obj *var = func->params; var; var = var->next)
      store_gp(i++, var->offset, var->type->size);

    // Emit code
    gen_statement(func->body);
    assert(depth == 0);

    println(".L.return.%s:", func->name);

    // Tear down stack frame
    println("  mov %%rbp, %%rsp"); // Reset stack pointer
    println("  pop %%rbp"); // Restore caller's base pointer
    println("  ret");
  }
}

void gen_asm(Obj *program, FILE *out) {
  output_file = out;

  assign_lvar_offsets(program);
  emit_data(program);
  emit_text(program);
}
