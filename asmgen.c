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

  // When loading a char or short to a reg, always extend to size of int, so we can assume the lower half of the reg always contains a valid value
  // The upper half of a reg for char, short, and int may contain garbage. When we load long, it just occupies the entire reg. 
  if (type->size == 1)
    println("  movsbl (%%rax), %%eax");
  else if (type->size == 2)
    println(" movswl (%%rax), %%eax");
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

static void cmp_zero(Type *type) {
  if (is_integer(type) && type->size <= 4)
    println("  cmp $0, %%eax");
  else
    println("  cmp $0, %%rax");
}

enum Bit { I8, I16, I32, I64 };

static int getTypeId(Type *type) {
  switch (type->kind) {
    case TY_CHAR:
      return I8;
    case TY_SHORT:
      return I16;
    case TY_INT:
      return I32;
  }
  return I64;
}

// Table for type casts
static char i32i8[] = "movsbl %al, %eax";
static char i32i16[] = "movswl %ax, %eax";
static char i32i64[] = "movsxd %eax, %rax";

static char *cast_table[][10] = {
  {NULL,  NULL,   NULL, i32i64}, // i8
  {i32i8, NULL,   NULL, i32i64}, // i16
  {i32i8, i32i16, NULL, i32i64}, // i32
  {i32i8, i32i16, NULL, NULL},   // i64
};

static void cast(Type *from, Type *to) {
  if (to->kind == TY_VOID)
    return;
  if (to->kind == TY_BOOL) {
    cmp_zero(from);
    println("  setne %%al");
    println("  movzx %%al, %%eax");
  }

  int t1 = getTypeId(from);
  int t2 = getTypeId(to);
  if (cast_table[t1][t2])
    println("  %s", cast_table[t1][t2]);
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
    case ND_CAST:
      gen_expr(node->left);
      cast(node->left->type, node->type);
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

  char *ax, *di;

  if (node->left->type->kind == TY_LONG || node->left->type->base) {
    ax = "%rax";
    di = "%rdi";
  } else {
    ax = "%eax";
    di = "%edi";
  }

  switch(node->node_type) {
    case ND_ADD:
      println("  add %s, %s", di, ax);
      return;
    case ND_SUB:
      println("  sub %s, %s", di, ax);
      return;
    case ND_MUL:
      println("  imul %s, %s", di, ax);
      return;
    case ND_DIV:
      // 64-bit instruction
      if (node->left->type->size == 8)
        println("  cqo");
      // 32-bit instruction
      else
        println(" cdq");
      println("  idiv %s", di);
      return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
      println("  cmp %s, %s", di, ax);

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

    if (func->is_static)
      println("  .local %s", func->name);
    else 
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
