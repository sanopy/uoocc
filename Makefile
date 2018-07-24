CC = gcc
TARGET = cc.out
CFLAGS = -std=c11 -Wall
SRCS = main.c vector.c map.c mylib.c lex.c
OBJS := $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS)

.PHONY: clean
clean:
	$(RM) $(TARGET) $(OBJS) *.s *.out

utiltest.out: vector.o map.o mylib.o test/test_utils.c
	gcc -o $@ $^

.PHONY: test
test: cc.out utiltest.out
	./utiltest.out
	./test/test_main.sh

.PHONY: format
format:
	clang-format -i *.c **/*.c *.h
