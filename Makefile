CFLAGS=-std=c11 -g -fno-common

SRCS = $(wildcard *.c) 

OBJS = $(SRCS:.c=.o)

TEST_SRCS = $(filter-out test/testfile.c, $(wildcard test/*.c))
TESTS = $(TEST_SRCS:.c=.exe)

default : main test clean

main : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJS): token.h

test/%.exe: main test/%.c
				$(CC) -o- -E -P -C test/$*.c | ./main -o test/$*.s -
				$(CC) -o $@ test/$*.s -xc test/common

test: $(TESTS)
	for i in $^; do echo $$i; ./$$i || exit 1; echo; done
	test/driver.sh

clean:
	rm -rf main tmp* $(TESTS) test/*.s test/*.exe
	find * -type f '(' -name '*~' -o -name '*.o' ')' -exec rm {} ';'
