#!/bin/bash

total=0

assertEquals() {
  actual=$1
  expected=$2
  if [ "$actual" = "$expected" ]; then
    echo "Test $total: Passed"
  else
    echo "Test $total: Failed"
    echo -e "\tExpected: $expected, Actual: $actual"
    exit 1
  fi
  total=`echo "$total+1" | bc`
}

runtest() {
  echo "$1" | ./cc.out > test.s
  gcc test.s test/func.c -o test.out
  ./test.out
  assertEquals $? $2
  rm -f test.s test.out
}

printtest() {
  echo "$1" | ./cc.out > test.s
  gcc test.s test/func.c -o test.out
  assertEquals `./test.out` $2
  rm -f test.s test.out
}

failtest() {
  actual=`echo "$1" | ./cc.out 2>&1 >/dev/null`
  expected=$2
  len1=${#actual}
  len2=${#expected}
  assertEquals "${actual:$((len1-len2))}" "$expected"
}

echo '=== number ==='
runtest 'int main() { 0; }' 0
runtest 'int main() { 2; }' 2
runtest 'int main() { 9; }' 9

echo '=== additive expression ==='
echo " - integer"
runtest 'int main() { 1 + 2; }' 3
runtest 'int main() { 2 - 1; }' 1
runtest 'int main() { 1 + 2 + 3; }' 6
runtest 'int main() { 3 - 2 - 1; }' 0
runtest 'int main() { 5 + 6 - 3; }' 8
runtest 'int main() { 3 + 5 - 3 + 6; }' 11
echo " - pointer"
runtest 'int main() { int *p; alloc4(&p, 1, 2, 4, 8); *p; }' 1
runtest 'int main() { int *p; alloc4(&p, 1, 2, 4, 8); p = 2 + p; *p; }' 4
runtest 'int main() { int *p; alloc4(&p, 1, 2, 4, 8); p = p + 3; *p; }' 8
runtest 'int main() { int *p; alloc4(&p, 1, 2, 4, 8); p = p + 3; p = p - 2; *p; }' 2
runtest 'int main() { int *p; alloc4(&p, 1, 2, 4, 8); int *q; q = p + 3; q - p; }' 3

echo '=== multiplicative expression ==='
runtest 'int main() { 2 * 3; }' 6
runtest 'int main() { 4 / 2; }' 2
runtest 'int main() { 1 * 2 + 3; }' 5
runtest 'int main() { 1 + 2 * 3; }' 7
runtest 'int main() { 1 / 2 + 3; }' 3
runtest 'int main() { 1 + 2 / 3; }' 1
runtest 'int main() { 1 * 2 + 3 + 4 * 5 * 6 / 7; }' 22

echo "=== primary expression ==="
runtest 'int main() { 6 - (5 - 4); }' 5
runtest 'int main() { (1 + 2) * 3; }' 9
runtest 'int main() { (1 + 2) * 3 + (4 + 5 + 6) * 2; }' 39

echo "=== unary expression ==="
echo " - increment / decrement"
runtest 'int main() { int a; a = 1; ++a; }' 2
runtest 'int main() { int a; a = 1; ++a; a; }' 2
runtest 'int main() { int a; a = 3; --a; }' 2
runtest 'int main() { int a; a = 3; --a; a; }' 2
echo " - pointer"
runtest 'int main() { int *p; alloc4(&p, 1, 2, 4, 8); *(++p); }' 2
runtest 'int main() { int *p; alloc4(&p, 1, 2, 4, 8); p = p + 3; *(--p); }' 4
runtest 'int main() { int a; a = 7; int *p; p = &a; *p; }' 7
runtest 'int main() { int a; int *p; p = &a; *p = 15; *p; }' 15
runtest 'int main() { int a; a = 3; int *p; p = &a; *p = 9; a; }' 9
runtest 'int swap(int *p, int *q) { int tmp; tmp = *p; *p = *q; *q = tmp; }
int main() { int a; a = 5; int b; b = 9; swap(&a, &b); a; }' 9
runtest 'int swap(int *p, int *q) { int tmp; tmp = *p; *p = *q; *q = tmp; }
int main() { int a; a = 5; int b; b = 9; swap(&a, &b); b; }' 5

echo "=== postfix expression ==="
runtest 'int main() { int a; a = 1; a++; }' 1
runtest 'int main() { int a; a = 1; a++; a; }' 2
runtest 'int main() { int a; a = 3; a--; }' 3
runtest 'int main() { int a; a = 3; a--; a; }' 2
runtest 'int main() { int *p; alloc4(&p, 1, 2, 4, 8); *(p++); }' 1
runtest 'int main() { int *p; alloc4(&p, 1, 2, 4, 8); p++; *p; }' 2
runtest 'int main() { int *p; alloc4(&p, 1, 2, 4, 8); p = p + 3; *(p--); }' 8
runtest 'int main() { int *p; alloc4(&p, 1, 2, 4, 8); p = p + 3; p--; *p; }' 4

echo "=== relational expression ==="
runtest 'int main() { 3 < 5; }' 1
runtest 'int main() { 5 < 5; }' 0
runtest 'int main() { 3 <= 5; }' 1
runtest 'int main() { 5 <= 5; }' 1
runtest 'int main() { 5 <= 3; }' 0
runtest 'int main() { 5 > 3; }' 1
runtest 'int main() { 5 > 5; }' 0
runtest 'int main() { 5 >= 3; }' 1
runtest 'int main() { 5 >= 5; }' 1
runtest 'int main() { 3 >= 5; }' 0

echo "=== equality expression ==="
runtest 'int main() { 25 == 25; }' 1
runtest 'int main() { 10 + 5 == 3 * 5; }' 1
runtest 'int main() { 23 == 39; }' 0

echo "=== variable ==="
runtest 'int main() {int a; a = 1; a; }' 1
runtest 'int main() {int a; int b; a = 1; b = 2; a + b; }' 3
runtest 'int main() {int a; int b; int c; a = b = c = 1; a + b + c; }' 3
runtest 'int main() {int a; int b; int c; a = 5; b = a + 6; c = b * 2; a * 2 + b + c; }' 43

echo "=== function ==="
printtest 'int main() { print_hello(); }' 'hello'
runtest 'int main() { return_seven(); }' 7
runtest 'int main() { 2 * return_seven() + 5; }' 19
printtest 'int main() { print_n(15); }' '15'
printtest 'int main() { print_args(21,22,23,24,25,26,27,28); }' '21,22,23,24,25,26,27,28'
runtest 'int main() { 3 * add_three_args(1, 2, 3) / 2; }' 9
runtest 'int main() { int x; x = 7; add_eight_args(1, 2 * 2, 3, 4 + 4, 5 / 5 + 5, 6, x, 8 * 8 + 8); }' 107

echo "=== declare function ==="
runtest 'int add(int a, int b) { a + b; } int main() { add(3, 9); }' 12
runtest 'int test(int a, int b, int c, int d, int e, int f) {
  a + b * b + c + d + d + e / e + e + f;
}
int main() {
  int a;
  a = test(1, 2, 3, 4, 5, 6);
  a * 2 + 3;
}' 59

echo "=== if statement ==="
runtest 'int main() {
  if (1 == 2) {
    1;
  } else {
    2;
  }
}' 2
runtest 'int fact(int n) {
  if (n <= 1)
    1;
  else
    n * fact(n-1);
} int main() { fact(5); }' 120
runtest 'int fib(int n) {
  if (n <= 0)
    0;
  else if (n == 1)
    1;
  else
    fib(n-1) + fib(n-2);
} int main() { fib(8); }' 21

echo "=== while statement ==="
runtest 'int main() {
  int i;
  i = 1;
  while (i < 10)
    i++;
  i;
}' 10
runtest 'int main() {
  int i; int sum;
  sum = 0; i = 1;
  while (i <= 10)
    sum = sum + i++;
  sum;
}' 55

echo "=== for statement ==="
runtest 'int main() {
  int i;
  for (i = 0; i < 10; i++) i;
}' 9
runtest 'int main() {
  int i; int sum;
  sum = 0;
  for (i = 1; i <= 10; i++)
    sum = sum + i;
  sum;
}' 55

echo "=== fail test ==="
failtest '1;' "'int' was expected."
failtest 'int () {}' "ident was expected."
failtest 'int main) {}' "'(' was expected."
failtest 'int main( {}' "'int' was expected."
failtest 'int main() }' "'{' was expected."
failtest 'int main() {' "primary-expression was expected."
failtest 'int main() { 1 }' "';' was expected."
failtest 'int main() { 1 1; }' "';' was expected."
failtest 'int main() { 1 + ; }' "primary-expression was expected."
failtest 'int main() { (); }' "primary-expression was expected."
failtest 'int main() { (1 + 3; }' "')' was expected."
failtest 'int main() { 1 + 3); }' "';' was expected."
failtest 'int main() { 1 = 1; }' "expression is not assignable."
failtest 'int main() { ++1; }' "expression is not assignable."
failtest 'int main() { 1++; }' "expression is not assignable."
failtest 'int main() { int a; a++ = 1; }' "expression is not assignable."
failtest 'int main() { int a; ++a = 1; }' "expression is not assignable."
failtest 'int main() { int x; int x; }' "redifinition of 'x'."
failtest 'int main() { int 1; }' "ident was expected."
failtest 'int main() { x = 1; }' "use of undeclared identifier 'x'."
failtest 'int main() { int *p; p = 1; }' "expression is not assignable."
failtest 'int main() { int a; int *p; p = a; }' "expression is not assignable."
failtest 'int main() { int *p; **p; }' "indirection requires pointer operand."
failtest 'int main() { *1; }' "indirection requires pointer operand."
failtest 'int f(x) {}' "'int' was expected."
failtest 'int main(int 1) {}' "ident was expected."
failtest 'int f(int x, int x) {}' "redifinition of 'x'."
failtest 'int f(int a, int b, int c, int d, int e, int f, int g) {}' "too many arguments."
failtest 'int main() { if () {} }' "primary-expression was expected."
failtest 'int main() { while () {} }' "primary-expression was expected."
