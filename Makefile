CFLAGS= -Wall -Werror -g

CC = gcc

TARGET = main

OBJS = main.o

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

test: $(TARGET)
	./test.sh

clean:
	rm -f main *.o *~ tmp*

auto: test clean
