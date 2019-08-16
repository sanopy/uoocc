CC = gcc
TARGET = cc.out
CFLAGS = -std=c11 -Wall -g
SRCS = main.c vector.c map.c mylib.c lex.c parse.c analyze.c gen.c
OBJS := $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS)

.PHONY: clean
clean:
	$(RM) $(TARGET) $(OBJS) *.s *.out

utiltest.out: vector.o map.o mylib.o test/test_utils.c
	gcc -o $@ $^

.PHONY: test
test: cc.out utiltest.out format
	./uoocc test/expr.c test.out && ./test.out
	./uoocc test/func.c test.out && ./test.out
	./uoocc test/statement.c test.out && ./test.out
	./uoocc test/variable.c test.out && ./test.out
	rm -f test.out
	./utiltest.out
	./test/test_main.sh

.PHONY: format
format:
	clang-format -i *.c **/*.c *.h
