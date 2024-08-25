CFLAGS=-std=c11 -g -fno-common

SRCS = $(wildcard *.c) 

OBJS = $(SRCS:.c=.o)

main : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJS): token.h

test: main
	./test.sh
	./test-driver.sh

clean:
	rm -f main *.o *~ tmp*
