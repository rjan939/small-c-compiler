#! /bin/sh

make main

./main "$@" > tmp.s || exit
gcc -static -o tmp tmp.s
./tmp


