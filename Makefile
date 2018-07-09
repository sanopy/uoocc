cc.out: main.c
	gcc -std=c11 -Wall main.c -o cc.out

.PHONY: test
test: cc.out
	./test/test_main.sh

.PHONY: clean
clean:
	rm -f cc.out
