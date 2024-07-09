CFLAGS=-std=c11 -g -fno-common

SRCS = $(wildcard *.c) 

OBJS = $(SRCS:.c=.o)

main : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

temp: main.o
	$(CC) $(CFLAGS) -o main main.o

$(OBJS): token.h

test: main
	./test.sh

clean:
	rm -f main *.o *~ tmp*

auto: test clean
