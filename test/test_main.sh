#!/bin/sh

total=0

assertEquals() {
  actual=$1
  expected=$2
  if [ $actual = $expected ]; then
    echo "Test $total: Passed"
  else
    echo "Test $total: Failed"
    echo "\tExpected: $expected, Actual: $actual"
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

echo '=== number ==='
runtest 'main() { 0; }' 0
runtest 'main() { 2; }' 2
runtest 'main() { 9; }' 9

echo '=== additive expression ==='
runtest 'main() { 1 + 2; }' 3
runtest 'main() { 2 - 1; }' 1
runtest 'main() { 1 + 2 + 3; }' 6
runtest 'main() { 3 - 2 - 1; }' 0
runtest 'main() { 5 + 6 - 3; }' 8
runtest 'main() { 3 + 5 - 3 + 6; }' 11

echo '=== multiplicative expression ==='
runtest 'main() { 2 * 3; }' 6
runtest 'main() { 4 / 2; }' 2
runtest 'main() { 1 * 2 + 3; }' 5
runtest 'main() { 1 + 2 * 3; }' 7
runtest 'main() { 1 / 2 + 3; }' 3
runtest 'main() { 1 + 2 / 3; }' 1
runtest 'main() { 1 * 2 + 3 + 4 * 5 * 6 / 7; }' 22

echo "=== primary expression ==="
runtest 'main() { 6 - (5 - 4); }' 5
runtest 'main() { (1 + 2) * 3; }' 9
runtest 'main() { (1 + 2) * 3 + (4 + 5 + 6) * 2; }' 39

echo "=== unary expression ==="
runtest 'main() { a = 1; ++a; }' 2
runtest 'main() { a = 1; ++a; a; }' 2
runtest 'main() { a = 3; --a; }' 2
runtest 'main() { a = 3; --a; a; }' 2

echo "=== postfix expression ==="
runtest 'main() { a = 1; a++; }' 1
runtest 'main() { a = 1; a++; a; }' 2
runtest 'main() { a = 3; a--; }' 3
runtest 'main() { a = 3; a--; a; }' 2

echo "=== relational expression ==="
runtest 'main() { 3 < 5; }' 1
runtest 'main() { 5 < 5; }' 0
runtest 'main() { 3 <= 5; }' 1
runtest 'main() { 5 <= 5; }' 1
runtest 'main() { 5 <= 3; }' 0
runtest 'main() { 5 > 3; }' 1
runtest 'main() { 5 > 5; }' 0
runtest 'main() { 5 >= 3; }' 1
runtest 'main() { 5 >= 5; }' 1
runtest 'main() { 3 >= 5; }' 0

echo "=== equality expression ==="
runtest 'main() { 25 == 25; }' 1
runtest 'main() { 10 + 5 == 3 * 5; }' 1
runtest 'main() { 23 == 39; }' 0

echo "=== variable ==="
runtest 'main() {a = 1; a; }' 1
runtest 'main() {a = 1; b = 2; a + b; }' 3
runtest 'main() {a = b = c = 1; a + b + c; }' 3
runtest 'main() {a = 5; b = a + 6; c = b * 2; a * 2 + b + c; }' 43

echo "=== function ==="
printtest 'main() { print_hello(); }' 'hello'
runtest 'main() { return_seven(); }' 7
runtest 'main() { 2 * return_seven() + 5; }' 19
printtest 'main() { print_n(15); }' '15'
printtest 'main() { print_args(21,22,23,24,25,26,27,28); }' '21,22,23,24,25,26,27,28'
runtest 'main() { 3 * add_three_args(1, 2, 3) / 2; }' 9
runtest 'main() { x = 7; add_eight_args(1, 2 * 2, 3, 4 + 4, 5 / 5 + 5, 6, x, 8 * 8 + 8); }' 107

echo "=== declare function ==="
runtest 'add(a, b) { a + b; } main() { add(3, 9); }' 12
runtest 'test(a, b, c, d, e, f) { a + b * b + c + d + d + e / e + e + f; } main() { a = test(1, 2, 3, 4, 5, 6); a * 2 + 3; }' 59

echo "=== if statement ==="
runtest 'main() {
  if (1 == 2) {
    1;
  } else {
    2;
  }
}' 2
runtest 'fact(n) {
  if (n <= 1)
    1;
  else
    n * fact(n-1);
} main() { fact(5); }' 120
runtest 'fib(n) {
  if (n <= 0)
    0;
  else if (n == 1)
    1;
  else
    fib(n-1) + fib(n-2);
} main() { fib(8); }' 21

echo "=== while statement ==="
runtest 'main() {
  i = 1;
  while (i < 10)
    i++;
  i;
}' 10
runtest 'main() {
  sum = 0; i = 1;
  while (i <= 10)
    sum = sum + i++;
  sum;
}' 55