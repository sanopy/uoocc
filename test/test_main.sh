#!/bin/sh

total=0

runtest() {
  echo "$1" | ./cc.out > test.s
  gcc test.s -o test.out
  ./test.out
  actual=$?
  expected=$2
  if [ $actual = $expected ]; then
    echo "Test $total: Passed"
  else
    echo "Test $total: Failed"
    echo "\tExpected: $expected, Actual: $actual"
    exit 1
  fi
  total=`echo "$total+1" | bc`
  rm -f test.s test.out
}

echo '=== number ==='
runtest '0;' 0
runtest '2;' 2
runtest '9;' 9

echo '=== expression (+, - only) ==='
runtest '1 + 2;' 3
runtest '2 - 1;' 1
runtest '1 + 2 + 3;' 6
runtest '3 - 2 - 1;' 0
runtest '5 + 6 - 3;' 8
runtest '3 + 5 - 3 + 6;' 11

echo '=== expression (include *, /) ==='
runtest '2 * 3;' 6
runtest '4 / 2;' 2
runtest '1 * 2 + 3;' 5
runtest '1 + 2 * 3;' 7
runtest '1 / 2 + 3;' 3
runtest '1 + 2 / 3;' 1
runtest '1 * 2 + 3 + 4 * 5 * 6 / 7;' 22

echo "=== expression (include '(', ')') ==="
runtest '6 - (5 - 4);' 5
runtest '(1 + 2) * 3;' 9
runtest '(1 + 2) * 3 + (4 + 5 + 6) * 2;' 39

echo "=== variable ==="
runtest 'a = 1; a;' 1
runtest 'a = 1; b = 2; a + b;' 3
runtest 'a = b = c = 1; a + b + c;' 3
runtest 'a = 5; b = a + 6; c = b * 2; a * 2 + b + c;' 43
