#include "test.h"

int main() {
  ASSERT(97, 'a');
  ASSERT(10, '\n');
  ASSERT(-128, '\x80');

  printf("\nEVERYTHING GOOD\n");
  return 0;
}