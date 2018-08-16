CC = gcc
TARGET = cc.out
CFLAGS = -std=c11 -Wall
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
	cat test/expr.c | ./cc.out > test.s && gcc -static test.s -o test.out && ./test.out
	cat test/func.c | ./cc.out > test.s && gcc -static test.s -o test.out && ./test.out
	cat test/statement.c | ./cc.out > test.s && gcc -static test.s -o test.out && ./test.out
	cat test/variable.c | ./cc.out > test.s && gcc -static test.s -o test.out && ./test.out
	rm -f test.s test.out
	./utiltest.out
	./test/test_main.sh

.PHONY: format
format:
	clang-format -i *.c **/*.c *.h
