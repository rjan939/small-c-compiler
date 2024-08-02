#!/bin/bash
cat <<EOF | gcc -xc -c -o tmp2.o -
int ret3() { return 3; }
int ret5() { return 5; }
EOF

cleanup() {
  make clean
}

trap cleanup EXIT

assert() {
  expected="$1"
  input="$2"

  ./main "$input" > tmp.s || exit
  gcc -static -o tmp tmp.s tmp2.o
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then 
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 '{return 0;}'
assert 51 '{return 51;}'

assert 21 '{return 5+20-4;}'
assert 41 '{return 12 + 34 - 5 ;}'

assert 47 '{return 5+6*7;}'
assert 15 '{return 5*(9-6);}'
assert 4 '{return (3+5)/2;}'

assert 10 '{return -10+20;}'
assert 10 '{return - -10;}'
assert 10 '{return - - +10;}'

assert 0 '{return 0==1;}'
assert 1 '{return 51==51;}' 
assert 1 '{return 0!=1;}'
assert 0 '{return 51!=51;}'

assert 1 '{return 0<1;}'
assert 0 '{return 1>1;}'
assert 0 '{return 2<1;}'
assert 1 '{return 0<=1;}'
assert 1 '{return 1<=1;}'
assert 0 '{return 2<=1;}'

assert 1 '{return 1>0;}'
assert 0 '{return 1>1;}'
assert 0 '{return 1>2;}'
assert 1 '{return 1>=0;}'
assert 1 '{return 1>=1;}'
assert 0 '{return 1>=2;}'

assert 5 '{int a; a = 5; return a;}'
assert 5 '{int a = 5; return a; }'
assert 9 '{int a = 2; int z = 7; return a + z;}'
assert 6 '{int a; int b; a = b = 3; return a+b;}'
assert 2 '{int skibidi = 2; return skibidi;}'
assert 8 '{int skibidi123 = 3; int variable = 5; return skibidi123 + variable;}'

assert 3 '{1; 2; return 3;}'
assert 2 '{1; return 2; 3;}'
assert 1 '{return 1; 2; 3;}'

assert 3 '{{1; {2;} return 3;}}'
assert 5 '{ ;;; return 5;}'

assert 3 '{ if (0) return 2; return 3; }'
assert 3 '{ if (1 - 1) return 2; return 3; }'

assert 69 '{ if (1) return 69; return 4; }'
assert 69 '{ if (2 - 1) return 69; return 4; }'
assert 4 '{ if (0) { 1; 2; return 3; } else { return 4; } }'
assert 3 '{ if (1) { 1; 2; return 3; } else { return 4; } }'

assert 55 '{int i = 0; int j = 0; for (i = 0; i <= 10; i = i + 1) j = i + j; return j; }'
assert 69 '{ for (;;) {return 69;} return 6; }'

assert 6 '{ int i = 0; while (i < 6) { i = i  + 1; } return i; }'

assert 3 '{ int x = 3; return *&x; }'
assert 3 '{ int x = 3; int *y = &x; int **z = &y; return **z; }'
# I actually have no idea if this is supposed to work since its technically ub in C
# Since I cant confirm whether variables are initialized right next to each other on every system
assert 5 '{ int x = 3; int y = 5; return *(&x + 1); }'
assert 3 '{ int x = 3; int y = 5; return *(&y - 1); }'
assert 5 '{ int x = 3; int y = 5; return *(&x - (-1)); }'
assert 5 '{ int x = 3; int *y = &x; *y = 5; return x; }'
assert 7 '{ int x = 3; int y = 5; *(&x + 1) = 7; return y; }'
assert 7 '{ int x = 3; int y = 5; *(&y - 2 + 1) = 7; return x; }'
assert 5 '{ int x = 3; return (&x + 2) - &x + 3; }'
assert 8 '{ int x, y; x = 3; y = 5; return x + y; }'
assert 8 '{ int x = 3, y = 5; return x + y; }'

assert 3 '{ return ret3(); }'
assert 5 '{ return ret5(); }'


echo -e "\nEVERYTHING GOOD\n"
