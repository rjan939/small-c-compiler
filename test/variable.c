#include "test.h"

int g1, arr[4];

int main() {
  ASSERT(5, ({ int a; a = 5; a; }));
  ASSERT(5, ({ int a = 5; a; }));
  ASSERT(9, ({ int a = 2; int z = 7; a + z; }));
  ASSERT(6, ({ int a; int b; a = b = 3; a + b; }));
  ASSERT(2, ({ int skibidi = 2; skibidi; }));
  ASSERT(8, ({ int skibidi123 = 3; int variable = 5; skibidi123 + variable; }));

  ASSERT(8, ({ int x; sizeof(x); }));
  ASSERT(8, ({ int x; sizeof x; }));
  ASSERT(8, ({ int *x; sizeof(x); }));
  ASSERT(32, ({ int x[4]; sizeof(x); }));
  ASSERT(96, ({ int x[3][4]; sizeof(x); }));
  ASSERT(32, ({ int x[3][4]; sizeof(*x); }));
  ASSERT(8, ({ int x[3][4]; sizeof(**x); }));
  ASSERT(9, ({ int x[3][4]; sizeof(**x) + 1; }));
  ASSERT(9, ({ int x[3][4]; sizeof **x + 1; }));
  ASSERT(8, ({ int x[3][4]; sizeof(**x + 1); }));
  ASSERT(8, ({ int x = 1; sizeof(x = 2); }));
  ASSERT(1, ({ int x = 1; sizeof(x = 2); x; }));

  ASSERT(0, g1);
  ASSERT(3, ({g1=3; g1;}));
  ASSERT(0, ({ arr[0] = 0; arr[1] = 1; arr[2] = 2; arr[3] = 3; arr[0]; }));
  ASSERT(1, ({ arr[0] = 0; arr[1] = 1; arr[2] = 2; arr[3] = 3; arr[1]; }));
  ASSERT(2, ({ arr[0] = 0; arr[1] = 1; arr[2] = 2; arr[3] = 3; arr[2]; }));
  ASSERT(3, ({ arr[0] = 0; arr[1] = 1; arr[2] = 2; arr[3] = 3; arr[3]; }));

  ASSERT(8, sizeof(g1));
  ASSERT(32, sizeof(arr));
  
  ASSERT(1, ({char x = 1; x; }));
  ASSERT(1, ({char x = 1; char y = 2; x; }));
  ASSERT(2, ({ char x = 1; char y = 2; y; }));

  ASSERT(1, ({ char x; sizeof(x); }));
  ASSERT(10, ({char x[10]; sizeof(x); }));

  ASSERT(2, ({ int x = 2; { int x = 3; } x;}));
  ASSERT(2, ({ int x = 2; { int x = 3; } int y = 4; x; }));
  ASSERT(3, ({ int x = 2; { x = 3; } x;}));

  ASSERT(15, ({ int x; int y; char z; char *a = &y; char *b = &z; b - a; }));
  ASSERT(1, ({int x; char y; int z; char *a = &y; char *b = &z; b - a; }));

  printf("EVERYTHING GOOD\n");
  return 0;
}