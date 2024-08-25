#include "test.h"

int main() {
  ASSERT(3, ({ int x; if (0) x = 2; else x = 3; x;}));
  ASSERT(3, ({ int x; if (1 - 1) x = 2; x = 3; x; }));

  ASSERT(69, ({ int x; if (1) x = 69; else x = 64; x; }));
  ASSERT(69, ({ int x; if (2 - 1) x = 69; else x = 64; x; }));

  ASSERT(55, ({ int i = 0; int j = 0; for (i = 0; i <= 10; i = i + 1) j = i + j; j; }));

  ASSERT(6, ({ int i = 0; while (i < 6) { i = i  + 1; } i; }));

  ASSERT(3, ({ 1; {2;} 3; }));
  ASSERT(5, ({ ;;; 5; }));

  ASSERT(10, ({ int i=0; while(i<10) i=i+1; i; }));
  ASSERT(55, ({ int i=0; int j=0; while(i<=10) {j=i+j; i=i+1;} j; }));
  printf("\nEVERYTHING GOOD\n");
  return 0;
}