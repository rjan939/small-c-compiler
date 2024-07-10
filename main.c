#include "token.h"

int main(int argc, char **argv) {
  if (argc != 2) 
    error("%s: invalid number of arguments", argv[0]);

  Token *token = tokenize(argv[1]);
  Node *node = parse(token);

  gen_asm(node);
  return 0;
}
