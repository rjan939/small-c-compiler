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

assert 0 '0;'
assert 51 '51;'

assert 21 '5+20-4;'
assert 41 ' 12 + 34 - 5 ;'

assert 47 '5+6*7;'
assert 15 '5*(9-6);'
assert 4 '(3+5)/2;'

assert 10 '-10+20;'
assert 10 '- -10;'
assert 10 '- - +10;'

assert 0 '0==1;'
assert 1 '51==51;'
assert 1 '0!=1;'
assert 0 '51!=51;'

assert 1 '0<1;'
assert 0 '1>1;'
assert 0 '2<1;'
assert 1 '0<=1;'
assert 1 '1<=1;'
assert 0 '2<=1;'

assert 1 '1>0;'
assert 0 '1>1;'
assert 0 '1>2;'
assert 1 '1>=0;'
assert 1 '1>=1;'
assert 0 '1>=2;'

assert 3 '1; 2; 3;'

assert 5 'a=5; a;'
assert 9 'a=2; z=7; a+z;'
assert 6 'a=b=3; a+b;'


echo EVERYTHING GOOD
