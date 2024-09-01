#!/bin/bash

cleanup() {
  rm -rf $tmp
}

tmp=`mktemp -d /tmp/compiler-test-XXXXXX`
trap cleanup INT TERM HUP EXIT
echo > $tmp/empty.c

check() {
  if [ $? -eq 0 ]; then
    echo "testing $1 ... passed"
  else
    echo "testing $1 ... failed"
    exit 1
  fi
}

# -o
rm -f $tmp/out
./main -o $tmp/out $tmp/empty.c
[ -f $tmp/out ]
check -o

# -- help
./main --help 2>&1 | grep -q main
check --help

echo -e "\nEVERYTHING GOOD\n"
