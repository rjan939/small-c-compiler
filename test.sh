#!/bin/bash

cleanup() {
  make clean
}

trap cleanup EXIT

assert() {
  expected="$1"
  input="$2"

  ./main "$input" > tmp.s || exit
  gcc -static -o tmp tmp.s
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

assert 5 '{a=5; return a;}'
assert 9 '{a=2; z=7; return a+z;}'
assert 6 '{a=b=3; return a+b;}'
assert 2 '{skibidi=2; return skibidi;}'
assert 8 '{skibidi123=3; variable=5; return skibidi123+variable;}'

assert 3 '{1; 2; return 3;}'
assert 2 '{1; return 2; 3;}'
assert 1 '{return 1; 2; 3;}'


echo -e "\nEVERYTHING GOOD\n"
