#include "token.h"

void free_memory(Token *token, Function *program) {
  free_token(token);
  free_lvar(program->locals);
  free_node(program->body);
  free(program);
}

int main(int argc, char **argv) {
  if (argc != 2) 
    error("%s: invalid number of arguments", argv[0]);

  Token *token = tokenize(argv[1]);
  Function *program = parse(token);

  gen_asm(program);
  free_memory(token, program);
  return 0;
}
