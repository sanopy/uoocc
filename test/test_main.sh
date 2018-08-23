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

failtest() {
  actual=`echo "$1" | ./cc.out 2>&1 >/dev/null`
  expected=$2
  len1=${#actual}
  len2=${#expected}
  assertEquals "${actual:$((len1-len2))}" "$expected"
}

echo "=== fail test ==="
failtest '1;' "type_specifier was expected."
failtest 'int () {}' "ident was expected."
failtest 'int main) {}' "';' was expected."
failtest 'int main( {}' "type_specifier was expected."
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
failtest 'int f(x) {}' "type_specifier was expected."
failtest 'int main(int 1) {}' "ident was expected."
failtest 'int f(int x, int x) {}' "redifinition of 'x'."
failtest 'int f(int a, int b, int c, int d, int e, int f, int g) {}' "too many arguments."
failtest 'int main() { if () {} }' "primary-expression was expected."
failtest 'int main() { while () {} }' "primary-expression was expected."
echo 'OK!'
